#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "hih613x.h"
#include "i2cbb.h"
#include "spi.h"
#include "time.h"
#include "types.h"
#include "util.h"

#define FILE_ID HIH613X_C

#define HIHALM_PIN      (GPIO_Pin_3) // Labeled as HIHALM -- (pin pulled, used as I2CBBSCL)
#define HIH_I2C_ADDRESS (0x27)
#define HIH_MEASURE_CMD (HIH_I2C_ADDRESS << 1)
#define HIH_READ_CMD    (HIH_MEASURE_CMD  + 1)

// These macros will only apply to the SPI version of the chip
#define SELECT_CHIP_HIH613X()   do { GPIOB->BSRRH |= 0x00000200; } while (0)
#define DESELECT_CHIP_HIH613X() do { GPIOB->BSRRL |= 0x00000200; } while (0)

typedef struct
{
  uint32  dontCare    : 2;
  uint32  temperature : 14;
  uint32  humidity    : 14;
  uint32  status      : 2;
} HIH613XData;

// Power profile voltage definitions, in HIHPowerProfile / HIHState order
static const double HIH_POWER_PROFILES[HIH_PROFILE_MAX][HIH_STATE_MAX] =
{ // Idle, Data Ready, Transmitting, Waiting, Reading
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {2.3, 3.3, 3.3, 3.3, 3.3},  // Low power idle profile
  {2.3, 2.3, 3.3, 3.3, 3.3},  // Low power idle/ready profile
  {2.3, 2.3, 3.3, 2.3, 3.3},  // Low power idle/ready/wait profile
  {2.3, 2.3, 2.3, 2.3, 2.3}   // Low power all, (may have comm errors)
};

static struct
{
  boolean     isInitialized;
  double      vDomain[HIH_STATE_MAX]; // The domain voltage for each state
  HIHState    state;
  HIH613XData currData;
  double      currTmp;
  double      currHum;
} sHIH613X;

/**************************************************************************************************\
* FUNCTION    HIH613X_init
* DESCRIPTION Initializes the HIH613X module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void HIH613X_init(void)
{
  Util_fillMemory(&sHIH613X, sizeof(sHIH613X), 0x00);
  HIH613X_setPowerProfile(HIH_PROFILE_STANDARD);  // Set all states to 3.3v
  sHIH613X.state = HIH_STATE_IDLE;
  HIH613X_setup(FALSE);
  sHIH613X.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    HIH613X_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the HIH613X sensor
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state of the I2CBB pins
\**************************************************************************************************/
boolean HIH613X_setup(boolean state)
{ 
  // @DRM: Determine the clock rate relative to the voltage here
  I2CBB_setup(state, I2CBB_CLOCK_RATE_101562);  // Now configure the I2C lines
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    HIH613X_getState
* DESCRIPTION Returns the current state of HIH613X
* PARAMETERS  None
* RETURNS     The current state of HIH613X
\**************************************************************************************************/
HIHState HIH613X_getState(void)
{
  return sHIH613X.state;
}

/**************************************************************************************************\
* FUNCTION    HIH613X_getStateAsWord
* DESCRIPTION Returns the current state of HIH613X
* PARAMETERS  None
* RETURNS     The current state of HIH613X
\**************************************************************************************************/
uint32 HIH613X_getStateAsWord(void)
{
  return (uint32)sHIH613X.state;
}

/**************************************************************************************************\
* FUNCTION    HIH613X_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double HIH613X_getStateVoltage(void)
{
  return sHIH613X.vDomain[sHIH613X.state];
}

/**************************************************************************************************\
* FUNCTION    HIH613X_setState
* DESCRIPTION Sets the internal state of HIH613X and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void HIH613X_setState(HIHState state)
{
  if (sHIH613X.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sHIH613X.state = state;
  Analog_setDomain(SPI_DOMAIN, TRUE, sHIH613X.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    HIH613X_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of HIH613X
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean HIH613X_setPowerState(HIHState state, double vDomain)
{
  if (state >= HIH_STATE_MAX)
    return FALSE;
  else if (vDomain > 5.0)
    return FALSE;
  else
    sHIH613X.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    HIH613X_setPowerProfile
* DESCRIPTION Sets all power states of the temperature/humidity sensor to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean HIH613X_setPowerProfile(HIHPowerProfile profile)
{
  uint32 state;
  if (profile >= HIH_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < HIH_STATE_MAX; state++)
    sHIH613X.vDomain[state] = HIH_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
 * FUNCTION    HIH613X_getHumidity
 * DESCRIPTION Returns the last humidity measurement
 * PARAMETERS  None
 * RETURNS     The last humidity measurement
 \**************************************************************************************************/
 double HIH613X_getHumidity(void)
 {
   return sHIH613X.currHum;
 }

 /**************************************************************************************************\
 * FUNCTION    HIH613X_getTemperature
 * DESCRIPTION Returns the last temperature measurement
 * PARAMETERS  None
 * RETURNS     the last temperature measurement
 \**************************************************************************************************/
 double HIH613X_getTemperature(void)
 {
   return sHIH613X.currTmp;
 }

 /**************************************************************************************************\
 * FUNCTION    HIH613X_notifyVoltageChange
 * DESCRIPTION Called when any other task changes the voltage of the domain
 * PARAMETERS  newVoltage: The voltage that the domain is now experiencing
 * RETURNS     Nothing
 \**************************************************************************************************/
 void HIH613X_notifyVoltageChange(double newVoltage)
 {
   if (newVoltage < 2.3)
     sHIH613X.state = HIH_STATE_IDLE;
 }

/**************************************************************************************************\
* FUNCTION    HIH613X_readTempHumidSPI
* DESCRIPTION Reads the temperature out of the sensor
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void HIH613X_readTempHumidSPI(void)
{
  int16 hihDataBuffer[2];

  SELECT_CHIP_HIH613X();
  SPI_read((uint8*)&hihDataBuffer, 4); // Note that SPI can only write at 800kHz
  DESELECT_CHIP_HIH613X();
}

/**************************************************************************************************\
* FUNCTION    HIH613X_readTempHumidI2CBB
* DESCRIPTION Interfaces with the HIH613X-000 (I2C) temperature andS humidity sensor
* PARAMETERS  measure - sends a measurement command
*             read - reads the temperature and humidity from the HIH613X
*             convert - converts the temperature and humidity bit counts into degF and %RH
* RETURNS     Nothing
* NOTES       1) If both measure and read are set then the operation will be done at once with a
*                delay occurring between the measurement and read
*             2) The results are stored in the HIH613X structure
\**************************************************************************************************/
HIHStatus HIH613X_readTempHumidI2CBB(boolean measure, boolean read, boolean convert)
{
  if (measure)
  {
    HIH613X_setState(HIH_STATE_SENDING_CMD);
    HIH613X_setup(TRUE);
    I2CBB_readData(HIH_MEASURE_CMD, NULL, 0); // send measure command
    HIH613X_setup(FALSE);
    HIH613X_setState(HIH_STATE_WAITING);
  }

  if (measure && read)
  {
    Time_delay(40000); // tMeasure ~= 36.65ms, but 40ms for reliability
    HIH613X_setState(HIH_STATE_DATA_READY);
  }

  if (read)
  {
    HIH613X_setState(HIH_STATE_READING);
    HIH613X_setup(TRUE);
    I2CBB_readData(HIH_READ_CMD, (uint8*)&sHIH613X.currData, sizeof(sHIH613X.currData));
    Util_swap32((uint32*)&sHIH613X.currData);
    HIH613X_setup(FALSE);
    HIH613X_setState(HIH_STATE_DATA_READY);
  }

  if (convert)
  {
    sHIH613X.currHum = (((double)sHIH613X.currData.humidity    / 16383.0) * 100.0);
    sHIH613X.currTmp = (((double)sHIH613X.currData.temperature / 16383.0) * 165.0) - 40.0;
    sHIH613X.currTmp = (sHIH613X.currTmp * 1.8) + 32.0;
  }
  return (HIHStatus)sHIH613X.currData.status;
}
