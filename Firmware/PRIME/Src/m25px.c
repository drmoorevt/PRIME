#include "stm32f4xx_hal.h"
#include "analog.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "m25px.h"
#include "powercon.h"

#define FILE_ID M25PX_C

// Can remove the wait by configuring the pins as push-pull, but risk leakage into the domain
#define SELECT_CHIP_OF()   do {                                                              \
                                 HAL_GPIO_WritePin(OF_CS_GPIO_Port, OF_CS_Pin, GPIO_PIN_RESET); \
                                 Time_delay(1);                                               \
                               } while (0)

#define DESELECT_CHIP_OF() do {                                                              \
                                 HAL_GPIO_WritePin(OF_CS_GPIO_Port, OF_CS_Pin, GPIO_PIN_SET); \
                                 Time_delay(1);                                               \
                               } while (0)

// Write/Erase times defined in microseconds
#define PAGE_WRITE_TIME      ((uint32)5000)
#define SUBSECTOR_ERASE_TIME ((uint32)150000)
#define SECTOR_ERASE_TIME    ((uint32)3000000)
#define BULK_ERASE_TIME      ((uint32)80000000)

#define MP25X_MANUFACTURER_ID     (0X20)
#define MP25X_MEMORY_TYPE         (0x71)
#define MP25X_MEMORY_CAPACITY_1MB (0x14)
#define MP25X_MEMORY_CAPACITY_2MB (0x15)
                               
#define M25PX_HIGH_SPEED_VMIN (2.7)
#define M25PX_LOW_SPEED_VMIN  (2.3)

typedef enum
{
  M25PX_UNPROTECT_GLOBAL = 0x00,
  M25PX_PROTECT_GLOBAL   = 0x3C
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

// Power profile voltage definitions, in M25PXPowerProfile / M25PXState order
// SPI operation at 25(rd)/50(wr/all)MHz for 2.3 > Vcc > 3.6, 33(rd)/75(wr/all)MHz for 2.7 > Vcc > 3.6
static const double M25PX_POWER_PROFILES[M25PX_PROFILE_MAX][M25PX_STATE_MAX] =
{ // Idle, Reading, Erasing, Writing, Waiting
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {3.0, 3.3, 3.3, 3.3, 3.0},  // 30VIW
  {2.7, 3.3, 3.3, 3.3, 2.7},  // 27VIW
  {2.3, 3.3, 3.3, 3.3, 2.3},  // 23VIW
  {2.1, 3.3, 3.3, 3.3, 2.1},  // 21VIW
};

static struct
{
  double              vDomain[M25PX_STATE_MAX]; // The vDomain for each state
  M25PXState    state;
  FlashStatusRegister status;
  M25PXSubSector      subSector;
  M25PXSubSector      testSubSector;
  boolean             isInitialized;
} sM25PX;

/**************************************************************************************************\
* FUNCTION    M25PX_init
* DESCRIPTION Initializes the M25PX module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void M25PX_init(void)
{
  Util_fillMemory((uint8*)&sM25PX, sizeof(sM25PX), 0x00);
  M25PX_setPowerProfile(M25PX_PROFILE_STANDARD);
  M25PX_setup(FALSE);
  sM25PX.state = M25PX_STATE_IDLE;
  sM25PX.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    M25PX_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the serial flash chip
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state if the SPI pins
\**************************************************************************************************/
boolean M25PX_setup(boolean state)
{
  DESELECT_CHIP_OF();

  // Set up the SPI transaction with respect to domain voltage
  if (sM25PX.vDomain[sM25PX.state] >= M25PX_HIGH_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_45000000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else if (sM25PX.vDomain[sM25PX.state] >= M25PX_LOW_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_22500000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else
  {
    SPI_setup(state, SPI_CLOCK_RATE_05625000, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
    return FALSE; // Domain voltage is too low for serial flash operation, attempt anyway
  }

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    M25PX_getStatePointer
* DESCRIPTION Returns the internal state of M25PX
* PARAMETERS  None
* RETURNS     The M25PXState
\**************************************************************************************************/
uint32_t *M25PX_getStatePointer(void)
{
  return (uint32_t *)sM25PX.state;
}

/**************************************************************************************************\
* FUNCTION    M25PX_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double M25PX_getStateVoltage(void)
{
  return sM25PX.vDomain[sM25PX.state];
}

/**************************************************************************************************\
* FUNCTION    M25PX_setState
* DESCRIPTION Sets the internal state of M25PX and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     The M25PXState
\**************************************************************************************************/
static void M25PX_setState(M25PXState state)
{
  if (sM25PX.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  
  VoltageDomain curDomain;
  switch (state)
  {
    case M25PX_STATE_IDLE:    curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for idle
    case M25PX_STATE_READING: curDomain = VOLTAGE_DOMAIN_2; break;  // MCU domain for reading
    case M25PX_STATE_WRITING: curDomain = VOLTAGE_DOMAIN_2; break;  // MCU domain for writing
    case M25PX_STATE_WAITING: curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for waiting
    default:                  curDomain = VOLTAGE_DOMAIN_2; break;  // Error...
  }
  PowerCon_setDeviceDomain(DEVICE_NORFLASH, curDomain);  // Move the device to the new domain
  PowerCon_setDomainVoltage(curDomain, sM25PX.vDomain[state]);  // Set the domain voltage
  sM25PX.state = state;
}

/**************************************************************************************************\
* FUNCTION    M25PX_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of M25PX
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean M25PX_setPowerState(M25PXState state, double vDomain)
{
  if (state >= M25PX_STATE_MAX)
    return FALSE;
  else if (vDomain > 3.6)
    return FALSE;
  else if (vDomain < 2.3)
    return FALSE;
  else
    sM25PX.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    M25PX_setPowerProfile
* DESCRIPTION Sets all power states of serial flash to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean M25PX_setPowerProfile(M25PXPowerProfile profile)
{
  uint32 state;
  if (profile >= M25PX_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < M25PX_STATE_MAX; state++)
    sM25PX.vDomain[state] = M25PX_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    M25PX_transceive
* DESCRIPTION Sends the data in the txBuffer and receives the data into the rxBuffer afterwards
* PARAMETERS  Obvious...
* RETURNS     None
\**************************************************************************************************/
void M25PX_transceive(uint8 *txBuffer, uint16 txLength, uint8 *rxBuffer, uint16 rxLength)
{
  M25PX_setup(TRUE);
  SELECT_CHIP_OF();
  SPI_write(txBuffer, txLength);
  if ((rxBuffer != NULL) && (rxLength > 0))
    SPI_read(rxBuffer, rxLength);
  DESELECT_CHIP_OF();
  M25PX_setup(FALSE);
}

/**************************************************************************************************\
* FUNCTION    M25PX_sendWriteEnable
* DESCRIPTION Sends the WREN command to the serial flash chip
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
void M25PX_sendWriteEnable(void)
{
  uint8 wrenCommand = OP_WRITE_ENABLE;
  M25PX_transceive(&wrenCommand, 1, NULL, 0);
}

/**************************************************************************************************\
* FUNCTION    M25PX_readM25PXFlashId
* DESCRIPTION Reads the Flash Identification from the M25PX16 device
* PARAMETERS  None
* RETURNS     The JEDEC Manufacturer, Device and Unique ID of the flash device
\**************************************************************************************************/
static M25PXFlashId M25PX_readFlashId(void)
{
  M25PXFlashId M25PXFlashId;
  uint8 readCmd = OP_READ_ID0;
  M25PX_transceive(&readCmd, 1, (uint8 *)&M25PXFlashId, sizeof(M25PXFlashId));
  return M25PXFlashId;
}

/**************************************************************************************************\
* FUNCTION    M25PX_readStatusRegister
* DESCRIPTION Reads the status register from flash
* PARAMETERS  None
* RETURNS     The flash status register
\**************************************************************************************************/
static FlashStatusRegister M25PX_readStatusRegister(void)
{
  uint8 readCmd = OP_READ_STATUS;
  FlashStatusRegister flashStatus;
  M25PX_transceive(&readCmd, 1, (uint8 *)&flashStatus, 1);
  return flashStatus;
}

/**************************************************************************************************\
* FUNCTION    M25PX_clearStatusRegister
* DESCRIPTION Clears the status register of flash
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static void M25PX_clearStatusRegister(void)
{
  uint8 writeCmd[2] = {OP_WRITE_STATUS, 0};
  M25PX_transceive(writeCmd, 2, NULL, 0);
}

/**************************************************************************************************\
* FUNCTION    M25PX_waitForWriteComplete
* DESCRIPTION Wait for the status register to transition to specified value or timeout
* PARAMETERS  pollChip: Routine will poll the status register for write complete indication
*             timeout:  Timeout in milliseconds
* RETURNS     TRUE if polling was not required, or if the timeout did not expire while polling
\**************************************************************************************************/
static boolean M25PX_waitForWriteComplete(boolean pollChip, uint32 timeout)
{
  SoftTimerConfig sfTimeout = {TIME_SOFT_TIMER_SERIAL_MEM, 0, 0, NULL};
  M25PX_setState(M25PX_STATE_WAITING);
  if (pollChip)
  {
    sfTimeout.value = timeout;
    Time_startTimer(sfTimeout);
    while (M25PX_readStatusRegister().writeInProgress && Time_getTimerValue(TIME_SOFT_TIMER_SERIAL_MEM));
    return (Time_getTimerValue(TIME_SOFT_TIMER_SERIAL_MEM) > 0);
  }
  else
  {
    Time_delay(timeout);
    return TRUE;
  }
}

/**************************************************************************************************\
* FUNCTION    M25PX_read
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in serial flash to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
M25PXResult M25PX_read(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 readCommand = ((uint32)pSrc & 0x00FFFFFF) | (OP_READ_MEMORY << 24);
  Util_swap32(&readCommand);

  M25PX_setState(M25PX_STATE_READING);
  M25PX_transceive((uint8 *)&readCommand, sizeof(readCommand), pDest, length);
  return M25PX_RESULT_OK;
}

/**************************************************************************************************\
* FUNCTION    M25PX_directWrite
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in RAM to write
*             pDest - pointer to destination in flash
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
static boolean M25PX_directWrite(uint8 *pSrc, uint8 *pDest, uint32 length, uint32 writeDelay)
{
  uint32 bytesToWrite, writeCommand;
  bool success;

  while (length > 0)
  {
    bytesToWrite = (length > M25PX_SIZE_PAGE) ? M25PX_SIZE_PAGE : length;
    writeCommand = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_WRITE_PAGE << 24);
    Util_swap32(&writeCommand);

    M25PX_setState(M25PX_STATE_WRITING);
    M25PX_sendWriteEnable();

    M25PX_setup(TRUE);
    SELECT_CHIP_OF();
    SPI_write((uint8 *)&writeCommand, sizeof(writeCommand));
    SPI_write(pSrc, bytesToWrite);
    DESELECT_CHIP_OF();
    M25PX_setup(FALSE);

    // Has the page write time been overriden?
    success  = (writeDelay > 0) ? M25PX_waitForWriteComplete(FALSE, writeDelay) 
                                : M25PX_waitForWriteComplete(FALSE, PAGE_WRITE_TIME);
    length -= bytesToWrite;
    pDest  += bytesToWrite;
    pSrc   += bytesToWrite;
  }

  return success;
}

/**************************************************************************************************\
* FUNCTION    M25PX_erase
* DESCRIPTION Erase a block of flash
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to erase
* RETURNS     status byte for serial flash
\**************************************************************************************************/
static boolean M25PX_erase(uint8 *pDest, M25PXSize size, uint32_t eraseDelay)
{
  uint32 eraseCmd = ((uint32)pDest & 0x00FFFFFF);
  uint32 timeout;
  bool success;

  // Put the erase command into the transmit buffer and set timeouts, both according to size
  switch (size)
  {
    case M25PX_SIZE_SUBSECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SUBSECTOR_ERASE << 24);
      timeout  = (SUBSECTOR_ERASE_TIME);
      break;
    case M25PX_SIZE_SECTOR:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_SECTOR_ERASE << 24);
      timeout  = (SECTOR_ERASE_TIME);
      break;
    case M25PX_SIZE_CHIP:
      eraseCmd = ((uint32)pDest & 0x00FFFFFF) | ((uint32)OP_BULK_ERASE << 24);
      timeout  = (BULK_ERASE_TIME);
      break;
    default:
      return FALSE;
  }
  Util_swap32(&eraseCmd);

  M25PX_setState(M25PX_STATE_ERASING);
  M25PX_sendWriteEnable();
  M25PX_transceive((uint8 *)&eraseCmd, sizeof(eraseCmd), NULL, 0);

  // Has the erase time been overriden?
  success  = (eraseDelay > 0) ? M25PX_waitForWriteComplete(FALSE, eraseDelay) 
                              : M25PX_waitForWriteComplete(FALSE, timeout);
  return success;
}

/*****************************************************************************\
* FUNCTION    M25PX_write
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in M25PX
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds
\*****************************************************************************/
M25PXResult M25PX_write(uint8 *pSrc, uint8 *pDst, uint32 len, uint32 eraseDelay, uint32 writeDelay)
{
  M25PXResult result = M25PX_RESULT_OK;
  uint8  *pCache   = (uint8 *)&sM25PX.subSector.byte[0];
  uint8  *pTestSub = (uint8 *)&sM25PX.testSubSector.byte[0];
  uint8  *pSubSector, *pCacheDest;
  uint8   retries;
  uint16  numToWrite;
  
  // Ensure that the chip has enough voltage and time to power up
  M25PX_setState(M25PX_STATE_IDLE);
  
  while ((result == M25PX_RESULT_OK) && (len > 0))
  {
    // Write must not go past a page boundary, but must erase a whole sub sector at a time
    numToWrite = M25PX_SIZE_PAGE - ((uint32)pDst & (M25PX_SIZE_PAGE - 1));
    if (len < numToWrite)
      numToWrite = len;

    // Determine the which subsector we'll be manipulating and the data destination in local cache
    pSubSector = (uint8 *)(((uint32)pDst  >> 0) & 0x001FF000);
    pCacheDest = (uint8 *)((((uint32)pDst >> 0) & 0x00000FFF) + pCache);

    // Read the sub sector to be written into local cache
    M25PX_read(pSubSector, pCache, M25PX_SIZE_SUBSECTOR);
    
    // Erase sub sector, it is now in local cache
    M25PX_erase(pSubSector, M25PX_SIZE_SUBSECTOR, eraseDelay);

    // Overwrite local cache with the source data at the specified destination
    Util_copyMemory(pSrc, pCacheDest, numToWrite);
    
    // Write the whole sub-sector back to flash
    M25PX_directWrite(pCache, pSubSector, M25PX_SIZE_SUBSECTOR, writeDelay);

    // Compare memory to determine if the write was successful
    M25PX_read(pSubSector, pTestSub, M25PX_SIZE_SUBSECTOR);
    
    if (0 != Util_compareMemory(pCache, pTestSub, M25PX_SIZE_SUBSECTOR))
      retries = 0;  // Will fail out (with line reset) on next line of code
      
    result = (retries == 0) ? M25PX_RESULT_ERROR : result;
    pSrc  += numToWrite; // update source pointer
    pDst  += numToWrite; // update destination pointer
    len   -= numToWrite;
  }
  M25PX_setState(M25PX_STATE_IDLE);
  return result;
}

/*****************************************************************************\
* FUNCTION    M25PX_test
* DESCRIPTION Executes reads and writes to M25PX to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
bool M25PX_test(void)
{
  M25PX_clearStatusRegister();
  M25PXFlashId flashId = M25PX_readFlashId();
  return ((flashId.manufacturerId == MP25X_MANUFACTURER_ID) &&
          (flashId.memoryType     == MP25X_MEMORY_TYPE)     &&
          (flashId.memoryCapacity == MP25X_MEMORY_CAPACITY_1MB));
}
