#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "serialflash.h"

#define FILE_ID SERIALFLASH_C

#define SERIAL_FLASH_PIN_HOLD   (GPIO_Pin_2)
#define SERIAL_FLASH_PIN_SELECT (GPIO_Pin_8)

// Can remove the wait by configuring the pins as push-pull, but risk leakage into the domain
#define SELECT_CHIP_SF()    do { GPIOB->BSRRH |= 0x00000100; Util_spinWait(60); } while (0)
#define DESELECT_CHIP_SF()  do { GPIOB->BSRRL |= 0x00000100; Util_spinWait(60); } while (0)
#define SELECT_SF_HOLD()    do { GPIOB->BSRRH |= 0x00000004; Util_spinWait(60); } while (0)
#define DESELECT_SF_HOLD()  do { GPIOB->BSRRL |= 0x00000004; Util_spinWait(60); } while (0)

// Write/Erase times defined in milliseconds
#define PAGE_WRITE_TIME      ((uint32)5)
#define SUBSECTOR_ERASE_TIME ((uint32)150)
#define SECTOR_ERASE_TIME    ((uint32)3000)
#define BULK_ERASE_TIME      ((uint32)80000)

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

typedef struct
{
  uint8 writeInProgress  : 1;
  uint8 writeEnableLatch : 1;
  uint8 blockProtectBits : 3;
  uint8 topBottomBit     : 1;
  uint8 zeroFill         : 1;
  uint8 statusRegWP      : 1;
} FlashStatusRegister;

// Power profile voltage definitions, in SerialFlashPowerProfile / SerialFlashState order
static const double SERIAL_FLASH_POWER_PROFILES[SERIAL_FLASH_PROFILE_MAX][SERIAL_FLASH_STATE_MAX] =
{
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {3.3, 3.3, 3.3, 3.3, 2.3},  // Low power wait profile
  {2.3, 2.3, 2.3, 2.3, 2.3},  // Low power all profile
  {3.3, 3.3, 3.3, 3.3, 1.8},  // Extreme low power wait profile
  {2.3, 2.3, 2.3, 2.3, 2.3}   // Low power all, extreme low power wait profile
};

static struct
{
  double           vDomain[SERIAL_FLASH_STATE_MAX]; // The vDomain for each state
  SerialFlashState state;
  FlashSubSector   subSector;
  FlashPage        testPage;
  boolean          isInitialized;
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
  Util_fillMemory((uint8*)&sSerialFlash, sizeof(sSerialFlash), 0x00);
  SerialFlash_setPowerProfile(SERIAL_FLASH_PROFILE_STANDARD);
  SerialFlash_setup(FALSE);
  sSerialFlash.state = SERIAL_FLASH_STATE_IDLE;
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
  GPIO_InitTypeDef sfCtrlPortB = {(SERIAL_FLASH_PIN_HOLD | SERIAL_FLASH_PIN_SELECT), GPIO_Mode_OUT,
                                   GPIO_Speed_25MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL,
                                   GPIO_AF_SYSTEM };
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
  if (state >= SERIAL_FLASH_STATE_MAX)
    return FALSE;
  else if (vDomain > 3.6)
    return FALSE;
  else if (vDomain < 2.3)
    return FALSE;
  else
    sSerialFlash.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SerialFlash_setPowerProfile
* DESCRIPTION Sets all power states of serial flash to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SerialFlash_setPowerProfile(SerialFlashPowerProfile profile)
{
  uint32 state;
  if (profile >= SERIAL_FLASH_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < SERIAL_FLASH_STATE_MAX; state++)
    sSerialFlash.vDomain[state] = SERIAL_FLASH_POWER_PROFILES[profile][state];
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
  SerialFlash_setState(SERIAL_FLASH_STATE_WAITING);
  SerialFlash_setup(pollChip);
  if (pollChip)
  {
    Time_startTimer(TIMER_SERIAL_MEM, timeout);
    while (SerialFlash_readStatusRegister().writeInProgress && Time_getTimerValue(TIMER_SERIAL_MEM));
    return (Time_getTimerValue(TIMER_SERIAL_MEM) > 0);
  }
  else
  {
    Util_spinDelay(timeout * 1000);
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
  uint32 readCommand = ((uint32)pSrc & 0x00FFFFFF) | (OP_READ_MEMORY << 24);
  Util_swap32(&readCommand);

  SerialFlash_setState(SERIAL_FLASH_STATE_READING);
  SerialFlash_setup(TRUE);
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
static boolean SerialFlash_directWrite(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 writeCommand = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_WRITE_PAGE << 24);
  Util_swap32(&writeCommand);

  SerialFlash_setState(SERIAL_FLASH_STATE_WRITING);
  SerialFlash_setup(TRUE);
  SerialFlash_sendWriteEnable();

  SELECT_CHIP_SF();
  SPI_write((uint8 *)&writeCommand, sizeof(writeCommand));
  SPI_write(pSrc, length);
  DESELECT_CHIP_SF();

  return SerialFlash_waitForWriteComplete(FALSE, PAGE_WRITE_TIME);
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
  uint8  *pCache = (uint8 *)&sSerialFlash.subSector.byte[0];
  uint8  *pSubSector, *pCacheDest;
  uint8   retries;
  boolean result = TRUE;
  uint16  numToWrite;

  while (result && (length > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = SERIAL_FLASH_SIZE_PAGE - ((uint32)pDest & (SERIAL_FLASH_SIZE_PAGE - 1));
    if (length < numToWrite)
      numToWrite = length;

    // Determine the which subsector we'll be manipulating and the data destination in local cache
    pSubSector = (uint8 *)(((uint32)pDest >> 12) & 0x000001FF);
    pCacheDest = (uint8 *)((((uint32)pDest >> 0) & 0x00000FFF) + pCache);

    for (retries = 3, result = TRUE; retries > 0; retries--)
    {
      // Read the sub sector to be written into local cache
      SerialFlash_read(pSubSector, pCache, SERIAL_FLASH_SIZE_SUBSECTOR);
      // Erase sub sector, it is now in local cache
      SerialFlash_erase(pSubSector, SERIAL_FLASH_SIZE_SUBSECTOR);
      // Overwrite local cache with the source data at the specified destination
      Util_copyMemory(pSrc, pCacheDest, numToWrite);
      // Write the whole sub-sector back to flash
      SerialFlash_directWrite(pCache, pSubSector, SERIAL_FLASH_SIZE_SUBSECTOR);
      // Compare memory to determine if the write was successful
      SerialFlash_read(pDest, sSerialFlash.testPage.byte, numToWrite);
      if (0 == Util_compareMemory(pSrc, sSerialFlash.testPage.byte, numToWrite))
        break;
      else
        Util_fillMemory(sSerialFlash.testPage.byte, numToWrite, 0xBA); // test code, remove
    }
    pSrc   += numToWrite; // update source pointer
    pDest  += numToWrite; // update destination pointer
    length -= numToWrite;
  }
  SerialFlash_setState(SERIAL_FLASH_STATE_IDLE);
  SerialFlash_setup(FALSE);
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
  uint32 eraseCmd = ((uint32)pDest & 0x00FFFFFF);
  boolean success;
  uint32 timeout;

  // Put the erase command into the transmit buffer and set timeouts, both according to size
  switch (size)
  {
    case SERIAL_FLASH_SIZE_SUBSECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SUBSECTOR_ERASE << 24);
      timeout  = (SUBSECTOR_ERASE_TIME);
      break;
    case SERIAL_FLASH_SIZE_SECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SECTOR_ERASE << 24);
      timeout  = (SECTOR_ERASE_TIME);
      break;
    case SERIAL_FLASH_SIZE_CHIP:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_BULK_ERASE << 24);
      timeout  = (BULK_ERASE_TIME);
      break;
    default:
      return FALSE;
  }
  Util_swap32(&eraseCmd);

  SerialFlash_setState(SERIAL_FLASH_STATE_ERASING);
  SerialFlash_setup(TRUE);
  SerialFlash_sendWriteEnable();
  SerialFlash_transceive((uint8 *)&eraseCmd, sizeof(eraseCmd), NULL, 0);
  success = SerialFlash_waitForWriteComplete(FALSE, timeout);

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
