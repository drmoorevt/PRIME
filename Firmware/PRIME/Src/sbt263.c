#include "analog.h"
#include "sbt263.h"
#include "i2c.h"
#include "powercon.h"
#include "time.h"
#include "types.h"
#include "util.h"
#include "stm32f4xx_hal.h"

#define FILE_ID SBT263_C

#define HIH_I2C_ADDRESS (0x27)
#define HIH_MEASURE_CMD (HIH_I2C_ADDRESS << 1)
#define HIH_READ_CMD    (HIH_MEASURE_CMD  + 1)

#define HIH_HIGH_SPEED_VMIN (2.7)
#define HIH_LOW_SPEED_VMIN (2.0)

// Power profile voltage definitions, in HIHPowerProfile / HIHState order
static const double SBT_POWER_PROFILES[SBT_PROFILE_MAX][SBT_STATE_MAX] =
{ // Idle, Data Ready, Transmitting, Waiting, Reading
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {2.9, 2.9, 2.9, 2.9, 3.3},  // 29VIRyTW
  {2.5, 2.5, 2.5, 2.5, 3.3},  // 25VIRyTW
  {2.3, 2.3, 2.3, 2.3, 3.3},  // 23VIRyTW
};

static struct
{
  boolean     isInitialized;
  double      vDomain[SBT_STATE_MAX]; // The domain voltage for each state
  SBTState    state;
  uint8_t     txBuf[1024];
  uint8_t     rxBuf[1024];
} sSBT263;

UART_HandleTypeDef *spUART;

/**************************************************************************************************\
* FUNCTION    SBT263_init
* DESCRIPTION Initializes the SBT263 module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SBT263_init(UART_HandleTypeDef *pUART)
{
  Util_fillMemory(&sSBT263, sizeof(sSBT263), 0x00);
  spUART = pUART;
  
  // Reset the device
  HAL_GPIO_WritePin(_BT_RESET_GPIO_Port, _BT_RESET_Pin, GPIO_PIN_RESET);
  Time_delay(1000000);
  HAL_GPIO_WritePin(_BT_RESET_GPIO_Port, _BT_RESET_Pin, GPIO_PIN_SET);
  
  SBT263_setPowerProfile(SBT_PROFILE_STANDARD);  // Set all states to 3.3v
  sSBT263.state = SBT_STATE_IDLE;
  SBT263_setup(FALSE);
  sSBT263.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    SBT263_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the SBT263
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state of the I2C pins
\**************************************************************************************************/
boolean SBT263_setup(boolean state)
{
  if (state)
    HAL_UART_Init(spUART);
  else
    HAL_UART_DeInit(spUART);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SBT263_getState
* DESCRIPTION Returns the current state of SBT263
* PARAMETERS  None
* RETURNS     The current state of SBT263
\**************************************************************************************************/
SBTState SBT263_getState(void)
{
  return sSBT263.state;
}

/**************************************************************************************************\
* FUNCTION    SBT263_getStateAsWord
* DESCRIPTION Returns the current state of SBT263
* PARAMETERS  None
* RETURNS     The current state of SBT263
\**************************************************************************************************/
uint32 SBT263_getStateAsWord(void)
{
  return (uint32)sSBT263.state;
}

/**************************************************************************************************\
* FUNCTION    SBT263_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double SBT263_getStateVoltage(void)
{
  return sSBT263.vDomain[sSBT263.state];
}

/**************************************************************************************************\
* FUNCTION    SBT263_setState
* DESCRIPTION Sets the internal state of SBT263 and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void SBT263_setState(SBTState state)
{
  if (sSBT263.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sSBT263.state = state;
  PowerCon_setDeviceDomain(DEVICE_TEMPSENSE, VOLTAGE_DOMAIN_0);
  //Analog_setDomain(SPI_DOMAIN, TRUE, sSBT263.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    SBT263_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of SBT263
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SBT263_setPowerState(SBTState state, double vDomain)
{
  if (state >= SBT_STATE_MAX)
    return FALSE;
  else if (vDomain > 5.0)
    return FALSE;
  else
    sSBT263.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SBT263_setPowerProfile
* DESCRIPTION Sets all power states of the temperature/humidity sensor to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SBT263_setPowerProfile(SBTPowerProfile profile)
{
  uint32 state;
  if (profile >= SBT_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < SBT_STATE_MAX; state++)
    sSBT263.vDomain[state] = SBT_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SBT263_notifyVoltageChange
* DESCRIPTION Called when any other task changes the voltage of the domain
* PARAMETERS  newVoltage: The voltage that the domain is now experiencing
* RETURNS     Nothing
\**************************************************************************************************/
void SBT263_notifyVoltageChange(double newVoltage)
{
  if (newVoltage < 2.3)
    sSBT263.state = SBT_STATE_IDLE;
}

/**************************************************************************************************\
* FUNCTION    SBT263_test
* DESCRIPTION Interfaces with the SBT263-000 (I2C) temperature andS humidity sensor
* PARAMETERS  measure - sends a measurement command
*             read - reads the temperature and humidity from the SBT263
*             convert - converts the temperature and humidity bit counts into degF and %RH
* RETURNS     Nothing
\**************************************************************************************************/
bool SBT263_test(void)
{
  uint32_t len;
  
  SBT263_setup(true);
  len = sprintf((char *)sSBT263.txBuf, "AT+AB Version\r\n");
  HAL_UART_Transmit(spUART, sSBT263.txBuf, len, 100);
  HAL_UART_Receive(spUART, sSBT263.rxBuf, sizeof(sSBT263.rxBuf), 1000);
  SBT263_setup(false);
  return (NULL != strstr((char *)sSBT263.rxBuf, "Ver"));
}
