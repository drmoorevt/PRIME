#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "eeprom.h"

#define FILE_ID EEPROM_C

#define EEPROM_PIN_HOLD   (GPIO_Pin_2)
#define EEPROM_PIN_SELECT (GPIO_Pin_5)

// Can remove the wait by configuring the pins as push-pull, but risk leakage into the domain
#define SELECT_CHIP_EE0()   do { GPIOB->BSRRH |= 0x00000020; } while (0)
#define DESELECT_CHIP_EE0() do { GPIOB->BSRRL |= 0x00000020; } while (0)
#define SELECT_EE_HOLD()    do { GPIOB->BSRRH |= 0x00000004; } while (0)
#define DESELECT_EE_HOLD()  do { GPIOB->BSRRL |= 0x00000004; } while (0)

#define WRITEPAGESIZE_EE ((uint16)128)
#define EE_NUM_RETRIES   (3)
#define ADDRBYTES_EE     (2)

#define EE_HIGH_SPEED_VMIN (4.5)
#define EE_MID_SPEED_VMIN  (2.5)
#define EE_LOW_SPEED_VMIN  (1.8)

typedef enum
{
  OP_WRITE_ENABLE = 0x06,
  OP_WRITE_MEMORY = 0x02,
  OP_READ_MEMORY  = 0x03,
  OP_WRITE_STATUS = 0x01,
  OP_READ_STATUS  = 0x05
} EEPROMCommand;

// Power profile voltage definitions, in SerialFlashPowerProfile / SerialFlashState order
// SPI operation at 20MHz for 4.5 < Vcc < 5.5, 10MHz for 2.5 < Vcc < 5.5, 2MHz for 1.8 < Vcc < 2.5
static const double EEPROM_POWER_PROFILES[EEPROM_PROFILE_MAX][EEPROM_STATE_MAX] =
{
  {3.3, 3.3, 3.3, 3.3},  // Standard profile, high speed high voltage
  {1.8, 3.3, 3.3, 1.8},  // Low power wait/idle profile
  {1.8, 2.5, 2.5, 1.8},  // Low power wait/idle, mid r/w profile
  {1.8, 1.8, 1.8, 1.8},  // Low power all profile
  {1.8, 1.8, 1.8, 1.5},  // Extreme low power wait profile
};

static struct
{
  double      vDomain[EEPROM_STATE_MAX]; // The domain voltage for each state
  EEPROMState state;
  boolean     isInitialized;
} sEEPROM;

/**************************************************************************************************\
* FUNCTION    EEPROM_init
* DESCRIPTION Initializes the EEPROM module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void EEPROM_init(void)
{
  Util_fillMemory((uint8*)&sEEPROM, sizeof(sEEPROM), 0x00);
  EEPROM_setPowerProfile(EEPROM_PROFILE_STANDARD);  // Set all states to 3.3v
  EEPROM_setup(FALSE);
  sEEPROM.state = EEPROM_STATE_IDLE;
  sEEPROM.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the EEPROM chip
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state if the SPI pins
\**************************************************************************************************/
boolean EEPROM_setup(boolean state)
{
  // Initialize the EEPROM chip select and hold lines
  GPIO_InitTypeDef eeCtrlPortB = {(EEPROM_PIN_HOLD | EEPROM_PIN_SELECT), GPIO_Mode_OUT,
                                   GPIO_Speed_25MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL,
                                   GPIO_AF_SYSTEM };
  eeCtrlPortB.GPIO_Mode = (state == TRUE) ? GPIO_Mode_OUT : GPIO_Mode_IN;
  GPIO_configurePins(GPIOB, &eeCtrlPortB);
  GPIO_setPortClock(GPIOB, TRUE);
  DESELECT_CHIP_EE0();
  DESELECT_EE_HOLD();

  // Set up the SPI transaction with respect to domain voltage
  if (sEEPROM.vDomain[sEEPROM.state] >= EE_HIGH_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_7500000);
  else if (sEEPROM.vDomain[sEEPROM.state] >= EE_MID_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_3250000);
  else if (sEEPROM.vDomain[sEEPROM.state] >= EE_LOW_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_812500);
  else
  {
    SPI_setup(state, SPI_CLOCK_RATE_406250);
    return FALSE; // Domain voltage is too low for EEPROM operation, attempt very low speed
  }

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_getState
* DESCRIPTION Returns the current state of EEPROM
* PARAMETERS  None
* RETURNS     The current state of EEPROM
\**************************************************************************************************/
EEPROMState EEPROM_getState(void)
{
  return sEEPROM.state;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_getStateAsWord
* DESCRIPTION Returns the current state of EEPROM
* PARAMETERS  None
* RETURNS     The current state of EEPROM
\**************************************************************************************************/
uint32 EEPROM_getStateAsWord(void)
{
  return (uint32)sEEPROM.state;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double EEPROM_getStateVoltage(void)
{
  return sEEPROM.vDomain[sEEPROM.state];
}

/**************************************************************************************************\
* FUNCTION    EEPROM_setState
* DESCRIPTION Sets the internal state of EEPROM and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void EEPROM_setState(EEPROMState state)
{
  if (sEEPROM.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sEEPROM.state = state;
  Analog_setDomain(SPI_DOMAIN, TRUE, sEEPROM.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    EEPROM_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of EEPROM
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean EEPROM_setPowerState(EEPROMState state, double vDomain)
{
  if (state >= EEPROM_STATE_MAX)
    return FALSE;
  else if (vDomain > 5.0)
    return FALSE;
  else
    sEEPROM.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_setPowerProfile
* DESCRIPTION Sets all power states of EEPROM to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean EEPROM_setPowerProfile(EEPROMPowerProfile profile)
{
  uint32 state;
  if (profile >= EEPROM_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < EEPROM_STATE_MAX; state++)
    sEEPROM.vDomain[state] = EEPROM_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_readEE
* DESCRIPTION Reads data out of EEPROM
* PARAMETERS  pSrc - pointer to data in EEPROM to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\**************************************************************************************************/
void EEPROM_read(const uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint8 cmd[ADDRBYTES_EE + 1];
  cmd[0] = OP_READ_MEMORY;
  cmd[1] = (uint8)((uint32)pSrc >> 8);
  cmd[2] = (uint8)((uint32)pSrc >> 0);

  // Set state before enabling the pins to avoid glitches
  EEPROM_setState(EEPROM_STATE_READING); // Set for monitoring and voltage control purposes
  EEPROM_setup(TRUE); // Enable the EEPROM control and SPI pins for the transaction

  SELECT_CHIP_EE0();
  SPI_write(cmd, ADDRBYTES_EE + 1);
  SPI_read(pDest, length);
  DESELECT_CHIP_EE0();

  EEPROM_setup(FALSE); // Disable the EEPROM control and SPI pins for the transaction
  EEPROM_setState(EEPROM_STATE_IDLE);
}

/**************************************************************************************************\
* FUNCTION    EEPROM_write
* DESCRIPTION Writes a buffer to EEPROM
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in EEPROM
*             length - number of bytes to write
* RETURNS     true if the write succeeds
\**************************************************************************************************/
EEPROMResult EEPROM_write(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint8 retries;
  uint16 numBytes;
  boolean writeFailed = FALSE;
  uint8 writeBuf[ADDRBYTES_EE + 1], readBuf[WRITEPAGESIZE_EE];

  while (length > 0)
  {
    // for EE, write must not go past a page boundary
    numBytes = WRITEPAGESIZE_EE - ((uint32)pDest & (WRITEPAGESIZE_EE-1));
    if (length < numBytes)
      numBytes = length;

    retries = EE_NUM_RETRIES; // change this to a for loop

    do
    {
      // Must set state (voltage) before setting control pins!
      EEPROM_setState(EEPROM_STATE_WRITING); // For monitoring and voltage control purposes
      EEPROM_setup(TRUE); // Enable the EEPROM control and SPI pins for the transaction

      // enable EEPROM
      SELECT_CHIP_EE0();
      writeBuf[0] = OP_WRITE_ENABLE;
      SPI_write(writeBuf, 1);
      DESELECT_CHIP_EE0();

      // write to EEPROM
      writeBuf[0] = OP_WRITE_MEMORY;
      writeBuf[1] = (uint8)((uint32)pDest >> 8);
      writeBuf[2] = (uint8)((uint32)pDest >> 0);

      SELECT_CHIP_EE0();
      SPI_write(writeBuf, ADDRBYTES_EE + 1);
      SPI_write(pSrc, numBytes);
      DESELECT_CHIP_EE0();

      EEPROM_setup(FALSE);  // Disable the EEPROM control and SPI pins while waiting
      EEPROM_setState(EEPROM_STATE_WAITING); // For monitoring and voltage control purposes
      Time_delay(5000); // EE_PAGE_WRITE_TIME

      EEPROM_read(pDest, readBuf, numBytes); // Verify the write, re-enables then disables EEPROM

      writeFailed = (Util_compareMemory(pSrc, readBuf, (uint8)numBytes) != 0);
    } while (writeFailed && (retries-- != 0));

    pSrc   += numBytes;  // update source pointer
    pDest  +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  return (writeFailed) ? EEPROM_RESULT_ERROR : EEPROM_RESULT_OK;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_fill
* DESCRIPTION Fills a section of EEPROM with the selected fillVal
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to zero
* RETURNS     nothing
\**************************************************************************************************/
boolean EEPROM_fill(uint8 *pDest, uint16 length, uint8 fillVal)
{
  uint8  fillBuf[WRITEPAGESIZE_EE];
  uint16 writeLength;
  uint8  i;
  
  for (i = 0; i < WRITEPAGESIZE_EE; i++)
    fillBuf[i] = fillVal;

  while (length)
  {
    writeLength = (length > sizeof(fillBuf)) ? sizeof(fillBuf) : length; // One page at a time
    if (EEPROM_write((uint8*)fillBuf, pDest, writeLength) == FALSE)
      return FALSE;  // Break early if any write fails
    else
    {
      length -= writeLength;  // Otherwise, continue the fill
      pDest  += writeLength;
    }
  }
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    EEPROM_test
* DESCRIPTION Executes reads and writes to EEPROM to test the code
* PARAMETERS  none
* RETURNS     nothing
\**************************************************************************************************/
void EEPROM_test(void)
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
  Time_delay(1000000); // Wait 1000ms for domains to settle

  // basic read test
  EEPROM_read((uint8*)0,test,sizeof(test));
  EEPROM_write(buffer,(uint8*)0,sizeof(buffer));
  EEPROM_read((uint8*)0,test,sizeof(test));

  // boundary test
  EEPROM_write(buffer,(uint8*)(8),sizeof(buffer));
  EEPROM_read((uint8*)(8),test,sizeof(test));

  while(1);
}
