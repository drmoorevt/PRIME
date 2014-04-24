#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "serialflash.h"

#define FILE_ID SERIALFLASH_C

#define SF_HOLD_PIN   (GPIO_Pin_2)
#define SF_SELECT_PIN (GPIO_Pin_8)

#define SF_NUM_RETRIES 3
#define ADDRBYTES_SF 3

#define SELECT_CHIP_SF()    do { GPIOB->BSRRH |= 0x00000100; } while (0)
#define DESELECT_CHIP_SF()  do { GPIOB->BSRRL |= 0x00000100; } while (0)

#define SELECT_SF_HOLD()    do { GPIOB->BSRRH |= 0x00000004; } while (0)
#define DESELECT_SF_HOLD()  do { GPIOB->BSRRL |= 0x00000004; } while (0)

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
  double           vDomain[SERIAL_FLASH_NUM_STATES]; // The vDomain for each state
  SerialFlashState state;
  FlashSubSector   subSector;
  FlashPage        testPage;
  boolean          isInitialized
} sSerialFlash;

static boolean SerialFlash_erase(uint8 *pDest, SerialFlashSize blockSize);
static FlashStatusRegister SerialFlash_readStatusRegister(void);

/**************************************************************************************************\
* FUNCTION    SerialFlash_init
* DESCRIPTION Initializes the SerialFlash module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SerialFlash_init(void)
{
  uint32 i;
  Util_fillMemory((uint8*)&sSerialFlash, sizeof(sSerialFlash), 0x00);
  for (i = 0; i < SERIAL_FLASH_NUM_STATES; i++)
    sSerialFlash.vDomain[i] = 3.3;  // Initialize the default of all states to operate at 3.3v
  sSerialFlash.state = SERIAL_FLASH_IDLE;
  SerialFlash_setup(FALSE);
  sSerialFlash.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the serial flash chip
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state if the SPI pins
\**************************************************************************************************/
boolean SerialFlash_setup(boolean state)
{
  // Initialize the EEPROM chip select and hold lines
  GPIO_InitTypeDef sfCtrlPortB = {(SF_HOLD_PIN | SF_SELECT_PIN), GPIO_Mode_OUT, GPIO_Speed_25MHz,
                                   GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  sfCtrlPortB.GPIO_Mode = (state == TRUE) ? GPIO_Mode_OUT : GPIO_Mode_IN;
  GPIO_configurePins(GPIOB, &sfCtrlPortB);
  GPIO_setPortClock(GPIOB, TRUE);
  DESELECT_CHIP_SF();
  DESELECT_SF_HOLD();
  SPI_setup(state);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_getState
* DESCRIPTION Returns the internal state of SerialFlash
* PARAMETERS  None
* RETURNS     The SerialFlashState
\**************************************************************************************************/
SerialFlashState SerialFlash_getState(void)
{
  return sSerialFlash.state;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_setState
* DESCRIPTION Sets the internal state of SerialFlash and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     The SerialFlashState
\**************************************************************************************************/
static void SerialFlash_setState(SerialFlashState state)
{
  if (sSerialFlash.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sSerialFlash.state = state;
  Analog_setDomain(SPI_DOMAIN, TRUE, sSerialFlash.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of SerialFlash
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SerialFlash_setPowerState(SerialFlashState state, double vDomain)
{
  if (state >= SERIAL_FLASH_NUM_STATES)
    return FALSE;
  else if (vDomain < 3.3)
    return FALSE;
  else
    sSerialFlash.vDomain[state] = vDomain;
  return TRUE;
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
* PARAMETERS  pollChip: Routine will poll the status register for write complete indication
*             timeout:  Timeout in milliseconds
* RETURNS     TRUE if polling was not required, or if the timeout did not expire while polling
\**************************************************************************************************/
static boolean SerialFlash_waitForWriteComplete(boolean pollChip, uint32 timeout)
{
  SerialFlash_setState(SERIAL_FLASH_WAITING);

  if (pollChip)
  {
    Time_startTimer(TIMER_SERIAL_MEM, timeout);
    while (SerialFlash_readStatusRegister().writeInProgress && Time_getTimerValue(TIMER_SERIAL_MEM));
    return (Time_getTimerValue(TIMER_SERIAL_MEM) > 0);
  }
  else
  {
    Util_spinWait(30000 * (timeout / 2));
    return TRUE;
  }
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
  SerialFlash_setState(SERIAL_FLASH_READING);
  SerialFlash_transceive((uint8 *)&readCommand, sizeof(readCommand), pDest, length);
  SerialFlash_setState(SERIAL_FLASH_IDLE);
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
  boolean success = TRUE;
  uint32 writeCommand, bytesToWrite;

  SerialFlash_setState(SERIAL_FLASH_WRITING);
  while (length > 0)
  {
    bytesToWrite = (length > SF_PAGE_SIZE) ? SF_PAGE_SIZE : length;
    writeCommand = (((uint32)pDest & 0xFFFFFF00) | (OP_WRITE_PAGE));
    SerialFlash_sendWriteEnable();
    SELECT_CHIP_SF();
    SPI_write((uint8 *)&writeCommand, sizeof(writeCommand));
    SPI_write(pSrc, bytesToWrite);
    DESELECT_CHIP_SF();
    success &= SerialFlash_waitForWriteComplete(FALSE, PAGE_WRITE_TIME);
    length -= bytesToWrite;
  }
  SerialFlash_setState(SERIAL_FLASH_IDLE);
  return success;
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
//      SPI_setup(FALSE, FALSE, TRUE, FALSE, TRUE);
      pSubSector = (uint8 *)(((uint32)pDest >> 12) & 0x000001FF);
      // Read the sub sector to be written into local cache
      SerialFlash_read(pSubSector, pCache, SF_SUBSECTOR_SIZE);
      // Erase sub sector, it is now in local cache
      SerialFlash_erase(pSubSector, SF_SUBSECTOR_SIZE);
      // Overwrite local cache with the source data at the specified destination
      Util_copyMemory(pSrc, pCache, length);
      // Write to flash
      SerialFlash_directWrite(pCache, pDest, SF_SUBSECTOR_SIZE);
      // Compare memory to determine if the write was successful
      SerialFlash_read(pDest, sSerialFlash.testPage.byte, sizeof(SF_PAGE_SIZE));
      if (0 == Util_compareMemory(pSrc, sSerialFlash.testPage.byte, numToWrite))
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
  boolean success;
  uint32 timeout, eraseCmd;

  SerialFlash_setState(SERIAL_FLASH_ERASING);
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
//  SPI_setup(FALSE, FALSE, TRUE, FALSE, FALSE);
  success = SerialFlash_waitForWriteComplete(FALSE, timeout);
  SerialFlash_setState(SERIAL_FLASH_IDLE);
//  SPI_setup(FALSE, FALSE, TRUE, FALSE, TRUE);
  return success;
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

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
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
