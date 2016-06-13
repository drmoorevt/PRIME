#include "stm32f4xx_hal.h"
#include "analog.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "sst26.h"
#include "powercon.h"

#define FILE_ID SST26_C

// Can remove the wait by configuring the pins as push-pull, but risk leakage into the domain
#define SELECT_CHIP_OF()   do {                                                              \
                                 HAL_GPIO_WritePin(AF_CS_GPIO_Port, AF_CS_Pin, GPIO_PIN_RESET); \
                                 Time_delay(1);                                               \
                               } while (0)

#define DESELECT_CHIP_OF() do {                                                              \
                                 HAL_GPIO_WritePin(AF_CS_GPIO_Port, AF_CS_Pin, GPIO_PIN_SET); \
                                 Time_delay(1);                                               \
                               } while (0)

// Write/Erase times defined in microseconds
#define PAGE_WRITE_TIME      ((uint32)5000)
#define SUBSECTOR_ERASE_TIME ((uint32)150000)
#define SECTOR_ERASE_TIME    ((uint32)3000000)
#define BULK_ERASE_TIME      ((uint32)80000000)

#define SST26_MANUFACTURER_ID     (0XBF)
#define SST26_MEMORY_TYPE         (0x26)
#define SST26_DEVICE_ID           (0x41)
                               
#define SST26_HIGH_SPEED_VMIN (2.7)
#define SST26_LOW_SPEED_VMIN  (2.3)

typedef enum
{
  SST26_UNPROTECT_GLOBAL = 0x00,
  SST26_PROTECT_GLOBAL   = 0x3C
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

// Power profile voltage definitions, in SST26PowerProfile / SST26State order
// SPI operation at 25(rd)/50(wr/all)MHz for 2.3 > Vcc > 3.6, 33(rd)/75(wr/all)MHz for 2.7 > Vcc > 3.6
static const double SST26_POWER_PROFILES[SST26_PROFILE_MAX][SST26_STATE_MAX] =
{ // Idle, Reading, Erasing, Writing, Waiting
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {3.0, 3.3, 3.3, 3.3, 3.0},  // 30VIW
  {2.7, 3.3, 3.3, 3.3, 2.7},  // 27VIW
  {2.3, 3.3, 3.3, 3.3, 2.3},  // 23VIW
  {2.1, 3.3, 3.3, 3.3, 2.1},  // 21VIW
};

static struct
{
  double              vDomain[SST26_STATE_MAX]; // The vDomain for each state
  SST26State    state;
  FlashStatusRegister status;
  SST26SubSector      subSector;
  SST26SubSector      testSubSector;
  boolean             isInitialized;
} sSST26;

static boolean SST26_erase(uint8 *pDest, SST26Size blockSize);
static FlashStatusRegister SST26_readStatusRegister(void);

/**************************************************************************************************\
* FUNCTION    SST26_init
* DESCRIPTION Initializes the SST26 module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SST26_init(void)
{
  Util_fillMemory((uint8*)&sSST26, sizeof(sSST26), 0x00);
  SST26_setPowerProfile(SST26_PROFILE_STANDARD);
  SST26_setup(FALSE);
  sSST26.state = SST26_STATE_IDLE;
  sSST26.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    SST26_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the serial flash chip
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state if the SPI pins
\**************************************************************************************************/
boolean SST26_setup(boolean state)
{
  DESELECT_CHIP_OF();

  // Set up the SPI transaction with respect to domain voltage
  if (sSST26.vDomain[sSST26.state] >= SST26_HIGH_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_45000000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else if (sSST26.vDomain[sSST26.state] >= SST26_LOW_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_22500000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else
  {
    SPI_setup(state, SPI_CLOCK_RATE_05625000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
    return FALSE; // Domain voltage is too low for serial flash operation, attempt anyway
  }

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SST26_getState
* DESCRIPTION Returns the internal state of SST26
* PARAMETERS  None
* RETURNS     The SST26State
\**************************************************************************************************/
SST26State SST26_getState(void)
{
  return sSST26.state;
}

/**************************************************************************************************\
* FUNCTION    SST26_getStateAsWord
* DESCRIPTION Returns the internal state of SST26
* PARAMETERS  None
* RETURNS     The SST26State
\**************************************************************************************************/
uint32 SST26_getStateAsWord(void)
{
  return (uint32)sSST26.state;
}

/**************************************************************************************************\
* FUNCTION    SST26_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double SST26_getStateVoltage(void)
{
  return sSST26.vDomain[sSST26.state];
}

/**************************************************************************************************\
* FUNCTION    SST26_setState
* DESCRIPTION Sets the internal state of SST26 and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     The SST26State
\**************************************************************************************************/
static void SST26_setState(SST26State state)
{
  if (sSST26.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sSST26.state = state;
  PowerCon_setDeviceDomain(DEVICE_NORFLASH, VOLTAGE_DOMAIN_0);
  //Analog_setDomain(SPI_DOMAIN, TRUE, sSST26.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    SST26_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of SST26
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SST26_setPowerState(SST26State state, double vDomain)
{
  if (state >= SST26_STATE_MAX)
    return FALSE;
  else if (vDomain > 3.6)
    return FALSE;
  else if (vDomain < 2.3)
    return FALSE;
  else
    sSST26.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SST26_setPowerProfile
* DESCRIPTION Sets all power states of serial flash to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SST26_setPowerProfile(SST26PowerProfile profile)
{
  uint32 state;
  if (profile >= SST26_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < SST26_STATE_MAX; state++)
    sSST26.vDomain[state] = SST26_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SST26_transceive
* DESCRIPTION Sends the data in the txBuffer and receives the data into the rxBuffer afterwards
* PARAMETERS  Obvious...
* RETURNS     None
\**************************************************************************************************/
void SST26_transceive(uint8 *txBuffer, uint16 txLength, uint8 *rxBuffer, uint16 rxLength)
{
  SST26_setup(TRUE);
  SELECT_CHIP_OF();
  SPI_write(txBuffer, txLength);
  if ((rxBuffer != NULL) && (rxLength > 0))
    SPI_read(rxBuffer, rxLength);
  DESELECT_CHIP_OF();
  SST26_setup(FALSE);
}

/**************************************************************************************************\
* FUNCTION    SST26_sendWriteEnable
* DESCRIPTION Sends the WREN command to the serial flash chip
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
void SST26_sendWriteEnable(void)
{
  uint8 wrenCommand = OP_WRITE_ENABLE;
  SST26_transceive(&wrenCommand, 1, NULL, 0);
}

/**************************************************************************************************\
* FUNCTION    SST26_readSST26FlashId
* DESCRIPTION Reads the Flash Identification from the SST2616 device
* PARAMETERS  None
* RETURNS     The JEDEC Manufacturer, Device and Unique ID of the flash device
\**************************************************************************************************/
static SST26FlashId SST26_readFlashId(void)
{
  SST26FlashId SST26FlashId;
  uint8 readCmd = OP_READ_ID0;
  SST26_transceive(&readCmd, 1, (uint8 *)&SST26FlashId, sizeof(SST26FlashId));
  return SST26FlashId;
}

/**************************************************************************************************\
* FUNCTION    SST26_readStatusRegister
* DESCRIPTION Reads the status register from flash
* PARAMETERS  None
* RETURNS     The flash status register
\**************************************************************************************************/
static FlashStatusRegister SST26_readStatusRegister(void)
{
  uint8 readCmd = OP_READ_STATUS;
  FlashStatusRegister flashStatus;
  SST26_transceive(&readCmd, 1, (uint8 *)&flashStatus, 1);
  return flashStatus;
}

/**************************************************************************************************\
* FUNCTION    SST26_clearStatusRegister
* DESCRIPTION Clears the status register of flash
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static void SST26_clearStatusRegister(void)
{
  uint8 writeCmd[2] = {OP_WRITE_STATUS, 0};
  SST26_transceive(writeCmd, 2, NULL, 0);
}

/**************************************************************************************************\
* FUNCTION    SST26_waitForWriteComplete
* DESCRIPTION Wait for the status register to transition to specified value or timeout
* PARAMETERS  pollChip: Routine will poll the status register for write complete indication
*             timeout:  Timeout in milliseconds
* RETURNS     TRUE if polling was not required, or if the timeout did not expire while polling
\**************************************************************************************************/
static boolean SST26_waitForWriteComplete(boolean pollChip, uint32 timeout)
{
  SoftTimerConfig sfTimeout = {TIME_SOFT_TIMER_SERIAL_MEM, 0, 0, NULL};
  SST26_setState(SST26_STATE_WAITING);
  if (pollChip)
  {
    sfTimeout.value = timeout;
    Time_startTimer(sfTimeout);
    while (SST26_readStatusRegister().writeInProgress && Time_getTimerValue(TIME_SOFT_TIMER_SERIAL_MEM));
    return (Time_getTimerValue(TIME_SOFT_TIMER_SERIAL_MEM) > 0);
  }
  else
  {
    Time_delay(timeout);
    return TRUE;
  }
}

/**************************************************************************************************\
* FUNCTION    SST26_read
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in serial flash to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
SST26Result SST26_read(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 readCommand = ((uint32)pSrc & 0x00FFFFFF) | (OP_READ_MEMORY << 24);
  Util_swap32(&readCommand);

  SST26_setState(SST26_STATE_READING);
  SST26_transceive((uint8 *)&readCommand, sizeof(readCommand), pDest, length);
  return SST26_RESULT_OK;
}

/**************************************************************************************************\
* FUNCTION    SST26_directWrite
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in RAM to write
*             pDest - pointer to destination in flash
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
static boolean SST26_directWrite(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 bytesToWrite, writeCommand;

  while (length > 0)
  {
    bytesToWrite = (length > SST26_SIZE_PAGE) ? SST26_SIZE_PAGE : length;
    writeCommand = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_WRITE_PAGE << 24);
    Util_swap32(&writeCommand);

    SST26_setState(SST26_STATE_WRITING);
    SST26_sendWriteEnable();

    SST26_setup(TRUE);
    SELECT_CHIP_OF();
    SPI_write((uint8 *)&writeCommand, sizeof(writeCommand));
    SPI_write(pSrc, bytesToWrite);
    DESELECT_CHIP_OF();
    SST26_setup(FALSE);

    SST26_waitForWriteComplete(FALSE, PAGE_WRITE_TIME);

    length -= bytesToWrite;
    pDest  += bytesToWrite;
    pSrc   += bytesToWrite;
  }

  return TRUE;
}

/*****************************************************************************\
* FUNCTION    SST26_write
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SST26
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds
\*****************************************************************************/
SST26Result SST26_write(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  SST26Result result = SST26_RESULT_OK;
  uint8  *pCache   = (uint8 *)&sSST26.subSector.byte[0];
  uint8  *pTestSub = (uint8 *)&sSST26.testSubSector.byte[0];
  uint8  *pSubSector, *pCacheDest;
  uint8   retries;
  uint16  numToWrite;
  
  // Ensure that the chip has enough voltage and time to power up
  SST26_setState(SST26_STATE_IDLE);
  
  while ((result != SST26_RESULT_ERROR) && (length > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = SST26_SIZE_PAGE - ((uint32)pDest & (SST26_SIZE_PAGE - 1));
    if (length < numToWrite)
      numToWrite = length;

    // Determine the which subsector we'll be manipulating and the data destination in local cache
    pSubSector = (uint8 *)(((uint32)pDest  >> 0) & 0x001FF000);
    pCacheDest = (uint8 *)((((uint32)pDest >> 0) & 0x00000FFF) + pCache);

    for (retries = 3; retries > 0; retries--)
    {
      // Read the sub sector to be written into local cache
      SST26_read(pSubSector, pCache, SST26_SIZE_SUBSECTOR);
      
      // Erase sub sector, it is now in local cache
      SST26_erase(pSubSector, SST26_SIZE_SUBSECTOR);

      // Overwrite local cache with the source data at the specified destination
      Util_copyMemory(pSrc, pCacheDest, numToWrite);
      
      // Write the whole sub-sector back to flash
      SST26_directWrite(pCache, pSubSector, SST26_SIZE_SUBSECTOR);

      // Compare memory to determine if the write was successful
      SST26_read(pSubSector, pTestSub, SST26_SIZE_SUBSECTOR);
      
      if (0 == Util_compareMemory(pCache, pTestSub, SST26_SIZE_SUBSECTOR))
        break;
      else
        result = SST26_RESULT_NEEDED_RETRY;
    }
    result  = (retries == 0) ? SST26_RESULT_ERROR : result;
    pSrc   += numToWrite; // update source pointer
    pDest  += numToWrite; // update destination pointer
    length -= numToWrite;
  }
  SST26_setState(SST26_STATE_IDLE);
  return result;
}

/**************************************************************************************************\
* FUNCTION    SST26_erase
* DESCRIPTION Erase a block of flash
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to erase
* RETURNS     status byte for serial flash
\**************************************************************************************************/
static boolean SST26_erase(uint8 *pDest, SST26Size size)
{
  uint32 eraseCmd = ((uint32)pDest & 0x00FFFFFF);
  boolean success;
  uint32 timeout;

  // Put the erase command into the transmit buffer and set timeouts, both according to size
  switch (size)
  {
    case SST26_SIZE_SUBSECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SUBSECTOR_ERASE << 24);
      timeout  = (SUBSECTOR_ERASE_TIME);
      break;
    case SST26_SIZE_SECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SECTOR_ERASE << 24);
      timeout  = (SECTOR_ERASE_TIME);
      break;
    case SST26_SIZE_CHIP:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_BULK_ERASE << 24);
      timeout  = (BULK_ERASE_TIME);
      break;
    default:
      return FALSE;
  }
  Util_swap32(&eraseCmd);

  SST26_setState(SST26_STATE_ERASING);
  SST26_sendWriteEnable();
  SST26_transceive((uint8 *)&eraseCmd, sizeof(eraseCmd), NULL, 0);
  success = SST26_waitForWriteComplete(FALSE, timeout);

  return success;
}

/*****************************************************************************\
* FUNCTION    SST26_test
* DESCRIPTION Executes reads and writes to SST26 to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
bool SST26_test(void)
{
  SST26_clearStatusRegister();
  SST26FlashId flashId = SST26_readFlashId();
  return ((flashId.manufacturerId == SST26_MANUFACTURER_ID) &&
          (flashId.memoryType     == SST26_MEMORY_TYPE)     &&
          (flashId.deviceId       == SST26_DEVICE_ID));
}
