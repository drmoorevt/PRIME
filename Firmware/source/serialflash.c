#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "serialflash.h"

#define FILE_ID SERIALFLASH_C

#define SF_NUM_RETRIES 3
#define ADDRBYTES_SF 3

#define SELECT_CHIP_SF()    do { GPIOB->BSRRH |= 0x00000100; } while (0)
#define DESELECT_CHIP_SF()  do { GPIOB->BSRRL |= 0x00000100; } while (0)

typedef enum
{
  SF_UNPROTECT_GLOBAL = 0x00,
  SF_PROTECT_GLOBAL   = 0x3C
} FlashGlobalProtect;

typedef enum
{
  OP_WRITE_ENABLE    = 0x06,  // WREN
  OP_WRITE_DISABLE   = 0x04,  // WRDI
  OP_READ_ID0        = 0x9F,  // RDID
  OP_READ_ID1        = 0x9E,  // RDID
  OP_READ_STATUS     = 0x05,  // RDSR
  OP_WRITE_STATUS    = 0x01,  // WRSR
  OP_WRITE_LOCK      = 0xE5,  // WRLR
  OP_READ_LOCK       = 0xE8,  // RDLR
  OP_READ_MEMORY     = 0x03,  // READ
  OP_FAST_READ       = 0x0B,  // FAST_READ
  OP_DUAL_READ       = 0x3B,  // DOFR
  OP_READ_OTP        = 0x4B,  // ROTP
  OP_WRITE_OTP       = 0x42,  // POTP
  OP_WRITE_PAGE      = 0x02,  // PP
  OP_DUAL_WRITE      = 0xA2,  // DIFP
  OP_SUBSECTOR_ERASE = 0x20,  // SSE
  OP_SECTOR_ERASE    = 0xD8,  // SE
  OP_BULK_ERASE      = 0xC7,  // BE
  OP_POWER_DOWN      = 0xB9,  // DP
  OP_POWER_UP        = 0xAB   // RDP
} FlashCommand;

//Serial flash status register
#define SF_SRP_bm 0x80
#define SF_TBB_bm 0x20
#define SF_BPX_bm 0x1C
#define SF_WEL_bm 0x02
#define SF_BSY_bm 0x01

typedef struct
{
  uint8 writeInProgress  : 1;
  uint8 writeEnableLatch : 1;
  uint8 blockProtectBits : 3;
  uint8 topBottomBit     : 1;
  uint8 zeroFill         : 1;
  uint8 statusRegWP      : 1;
} FlashStatusRegister;

#define PAGE_WRITE_TIME      ((uint32)5)
#define SUBSECTOR_ERASE_TIME ((uint32)150)
#define SECTOR_ERASE_TIME    ((uint32)3000)
#define BULK_ERASE_TIME      ((uint32)80000)

static struct
{
  SerialFlashState state;
  FlashSubSector   subSector;
  FlashPage        testPage;
} sSerialFlash;

static boolean SerialFlash_erase(uint8 *pDest, SerialFlashSize blockSize);
static FlashStatusRegister SerialFlash_readStatusRegister(void);

/*****************************************************************************\
* FUNCTION    SerialFlash_init
* DESCRIPTION Initializes the SerialFlash module
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
void SerialFlash_init(void)
{
  const uint16 sfCtrlPins = (GPIO_Pin_2 | GPIO_Pin_8);

  // Initialize the chip select and hold lines
  GPIO_InitTypeDef sfCtrlPortB = {sfCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &sfCtrlPortB);
  DESELECT_CHIP_SF();

  Util_fillMemory((uint8*)&sSerialFlash, sizeof(sSerialFlash), 0x00);
  sSerialFlash.state = SERIAL_FLASH_IDLE;
}

/*****************************************************************************\
* FUNCTION    SerialFlash_getState
* DESCRIPTION Returns the internal state of SerialFlash
* PARAMETERS  None
* RETURNS     The SerialFlashState
\*****************************************************************************/
SerialFlashState SerialFlash_getState(void)
{
  return sSerialFlash.state;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_transceive
* DESCRIPTION Sends the data in the txBuffer and receives the data into the rxBuffer afterwards
* PARAMETERS  Obvious...
* RETURNS     None
\**************************************************************************************************/
void SerialFlash_transceive(uint8 *txBuffer, uint16 txLength, uint8 *rxBuffer, uint16 rxLength)
{
  SELECT_CHIP_SF();
  SPI_write(txBuffer, txLength);
  if ((rxBuffer != NULL) && (rxLength > 0))
    SPI_read(rxBuffer, rxLength);
  DESELECT_CHIP_SF();
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_sendWriteEnable
* DESCRIPTION Sends the WREN command to the serial flash chip
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
void SerialFlash_sendWriteEnable(void)
{
  uint8 wrenCommand = OP_WRITE_ENABLE;
  SerialFlash_transceive(&wrenCommand, 1, NULL, 0);
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_readFlashID
* DESCRIPTION Reads the Flash Identification from the M25PX16 device
* PARAMETERS  None
* RETURNS     The JEDEC Manufacturer, Device and Unique ID of the flash device
\**************************************************************************************************/
FlashID SerialFlash_readFlashID(void)
{
  FlashID flashID;
  uint8 readCmd = OP_READ_ID0;
  SerialFlash_transceive(&readCmd, 1, (uint8 *)&flashID, 1);
  return flashID;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_readStatusRegister
* DESCRIPTION Reads the status register from flash
* PARAMETERS  None
* RETURNS     The flash status register
\**************************************************************************************************/
static FlashStatusRegister SerialFlash_readStatusRegister(void)
{
  uint8 readCmd = OP_READ_STATUS;
  FlashStatusRegister flashStatus;
  SerialFlash_transceive(&readCmd, 1, (uint8 *)&flashStatus, 1);
  return flashStatus;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_waitForWriteComplete
* DESCRIPTION Wait for the status register to transition to specified value or timeout
* PARAMETERS  timeout - timeout in ticks
* RETURNS     status byte for serial flash
\**************************************************************************************************/
static boolean SerialFlash_waitForWriteComplete(uint32 timeout)
{
  Time_startTimer(TIMER_SERIAL_MEM, timeout);
  while (SerialFlash_readStatusRegister().writeInProgress && Time_getTimerValue(TIMER_SERIAL_MEM));
  return (Time_getTimerValue(TIMER_SERIAL_MEM) > 0);
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_read
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in serial flash to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
boolean SerialFlash_read(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 readCommand = (((uint32)pSrc & 0xFFFFFF00) | (OP_READ_MEMORY));
  SerialFlash_transceive((uint8 *)&readCommand, sizeof(readCommand), pDest, length);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_directWrite
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in RAM to write
*             pDest - pointer to destination in flash
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
boolean SerialFlash_directWrite(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 writeCommand = (((uint32)pDest & 0xFFFFFF00) | (OP_WRITE_PAGE));

  SerialFlash_sendWriteEnable();

  SELECT_CHIP_SF();
  SPI_write((uint8 *)&writeCommand, sizeof(writeCommand));
  SPI_write(pSrc, length);
  DESELECT_CHIP_SF();

  return SerialFlash_waitForWriteComplete(PAGE_WRITE_TIME);
}

/*****************************************************************************\
* FUNCTION    SerialFlash_write
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SerialFlash
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds
\*****************************************************************************/
boolean SerialFlash_write(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint8  *pCache = (uint8 *)&sSerialFlash.subSector;
  uint8  *pSubSector;
  uint8   retries;
  boolean result = TRUE;
  uint16  numToWrite;

  while (result && (length > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = SF_PAGE_SIZE - ((uint32)pDest & (SF_PAGE_SIZE - 1));
    if (length < numToWrite)
      numToWrite = length;

    for (retries = SF_NUM_RETRIES, result = TRUE; (retries > 0); retries--)
    {
      pSubSector = (uint8 *)(((uint32)pDest >> 12) & 0x000001FF);
      // Read the sub sector to be written into local cache
      SerialFlash_read(pSubSector, pCache, sizeof(sSerialFlash.subSector));
      // Erase sub sector, it is now in local cache
      result &= SerialFlash_erase(pSubSector, SF_SUBSECTOR_SIZE);
      // Write to flash
      result &= SerialFlash_directWrite(pSrc, pDest, numToWrite);
      // Compare memory to determine if the write was successful
      SerialFlash_read(pDest, sSerialFlash.testPage.byte, sizeof(sSerialFlash.testPage));
      result &= !(Util_compareMemory(pSrc, sSerialFlash.testPage.byte, numToWrite));
      if (result)
        break;
    }
    pSrc   += numToWrite; // update source pointer
    pDest  += numToWrite; // update destination pointer
    length -= numToWrite;
  }
  return result;
}

/*************************************************************************************************\
* FUNCTION    SerialFlash_writeLP
* DESCRIPTION Writes a buffer to Serial Flash and decreases domain voltage during wait states
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SerialFlash
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds
\*************************************************************************************************/
boolean SerialFlash_writeLP(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint8  *pCache = (uint8 *)&sSerialFlash.subSector;
  uint8  *pSubSector;
  uint8   retries;
  boolean result = TRUE;
  uint16  numToWrite;

  while (result && (length > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = SF_PAGE_SIZE - ((uint32)pDest & (SF_PAGE_SIZE - 1));
    if (length < numToWrite)
      numToWrite = length;

    for (retries = SF_NUM_RETRIES, result = TRUE; (retries > 0); retries--)
    {
      pSubSector = (uint8 *)(((uint32)pDest >> 12) & 0x000001FF);
      // Read the sub sector to be written into local cache
      SerialFlash_read(pSubSector, pCache, sizeof(sSerialFlash.subSector));
      // Erase sub sector, it is now in local cache
      result &= SerialFlash_erase(pSubSector, SF_SUBSECTOR_SIZE);
      // Write to flash
      result &= SerialFlash_directWrite(pSrc, pDest, numToWrite);
      // Compare memory to determine if the write was successful
      SerialFlash_read(pDest, sSerialFlash.testPage.byte, sizeof(sSerialFlash.testPage));
      result &= !(Util_compareMemory(pSrc, sSerialFlash.testPage.byte, numToWrite));
      if (result)
        break;
    }
    pSrc   += numToWrite; // update source pointer
    pDest  += numToWrite; // update destination pointer
    length -= numToWrite;
  }
  return result;
}

/*****************************************************************************\
* FUNCTION    SerialFlash_writeXLP
* DESCRIPTION Writes a buffer to Serial Flash and decreases domain voltage during wait states and transmissions
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SerialFlash
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds
\*****************************************************************************/
boolean SerialFlash_writeXLP(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint8  *pCache = (uint8 *)&sSerialFlash.subSector;
  uint8  *pSubSector;
  uint8   retries;
  boolean result = TRUE;
  uint16  numToWrite;

  while (result && (length > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = SF_PAGE_SIZE - ((uint32)pDest & (SF_PAGE_SIZE - 1));
    if (length < numToWrite)
      numToWrite = length;

    for (retries = SF_NUM_RETRIES, result = TRUE; (retries > 0); retries--)
    {
      pSubSector = (uint8 *)(((uint32)pDest >> 12) & 0x000001FF);
      // Read the sub sector to be written into local cache
      SerialFlash_read(pSubSector, pCache, sizeof(sSerialFlash.subSector));
      // Erase sub sector, it is now in local cache
      result &= SerialFlash_erase(pSubSector, SF_SUBSECTOR_SIZE);
      // Write to flash
      result &= SerialFlash_directWrite(pSrc, pDest, numToWrite);
      // Compare memory to determine if the write was successful
      SerialFlash_read(pDest, sSerialFlash.testPage.byte, sizeof(sSerialFlash.testPage));
      result &= !(Util_compareMemory(pSrc, sSerialFlash.testPage.byte, numToWrite));
      if (result)
        break;
    }
    pSrc   += numToWrite; // update source pointer
    pDest  += numToWrite; // update destination pointer
    length -= numToWrite;
  }
  return result;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_erase
* DESCRIPTION Erase a block of flash
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to erase
* RETURNS     status byte for serial flash
\**************************************************************************************************/
static boolean SerialFlash_erase(uint8 *pDest, SerialFlashSize size)
{
  uint32 timeout, eraseCmd;

  SerialFlash_sendWriteEnable();

  // Put the erase command into the transmit buffer and set timeouts, both according to size
  switch (size)
  {
    case SF_SUBSECTOR_SIZE:
      eraseCmd = (((uint32)pDest & 0xFFFFFF00) | (OP_SUBSECTOR_ERASE));
      timeout  = SUBSECTOR_ERASE_TIME;
      break;
    case SF_SECTOR_SIZE:
      eraseCmd = (((uint32)pDest & 0xFFFFFF00) | (OP_SECTOR_ERASE));
      timeout = SECTOR_ERASE_TIME;
      break;
    case SF_CHIP_SIZE:
      eraseCmd = (((uint32)pDest & 0xFFFFFF00) | (OP_BULK_ERASE));
      timeout = BULK_ERASE_TIME;
      break;
    default:
      return FALSE;
  }
  SerialFlash_transceive((uint8 *)&eraseCmd, sizeof(eraseCmd), NULL, 0);

  // Wait for erase to complete depending on timeout
  return SerialFlash_waitForWriteComplete(timeout);
}

/*****************************************************************************\
* FUNCTION    SerialFlash_test
* DESCRIPTION Executes reads and writes to SerialFlash to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
void SerialFlash_test(void)
{
  uint8 buffer[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  uint8 test[sizeof(buffer)];

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(SPI_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustFeedbackVoltage(SPI_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 1000ms for domains to settle

  while(1)
  {
    // basic read test
    SerialFlash_read((uint8*)0,test,sizeof(test));
    SerialFlash_write(buffer,(uint8*)0,sizeof(buffer));
    SerialFlash_read((uint8*)0,test,sizeof(test));

    // boundary test
    SerialFlash_write(buffer,(uint8*)(8),sizeof(buffer));
    SerialFlash_read((uint8*)(8),test,sizeof(test));
  }
}
