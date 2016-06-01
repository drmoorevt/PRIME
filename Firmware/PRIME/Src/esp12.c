#include "analog.h"
#include "esp12.h"
#include "powercon.h"
#include "time.h"
#include "types.h"
#include "util.h"
#include "stm32f4xx_hal.h"

#define FILE_ID ESP12_C

// Power profile voltage definitions, in HIHPowerProfile / HIHState order
static const double ESP_POWER_PROFILES[ESP_PROFILE_MAX][ESP_STATE_MAX] =
{ // Idle, Data Ready, Transmitting, Waiting, Reading
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {2.9, 2.9, 2.9, 2.9, 3.3},  // 29VIRyTW
  {2.5, 2.5, 2.5, 2.5, 3.3},  // 25VIRyTW
  {2.3, 2.3, 2.3, 2.3, 3.3},  // 23VIRyTW
};

static struct
{
  boolean     isInitialized;
  double      vDomain[ESP_STATE_MAX]; // The domain voltage for each state
  ESPState    state;
  uint8_t     txBuf[256];
  uint8_t     rxBuf[256];
} sESP12;

static UART_HandleTypeDef *spUART;

/**************************************************************************************************\
* FUNCTION    ESP12_resetDevice
* DESCRIPTION Initializes the SBT263 module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static void ESP12_resetDevice(void)
{
  ESP12_setup(true);
  HAL_GPIO_WritePin(_WF_RESET_GPIO_Port, _WF_RESET_Pin, GPIO_PIN_RESET);
  Time_delay(1000000);
  HAL_GPIO_WritePin(_WF_RESET_GPIO_Port, _WF_RESET_Pin, GPIO_PIN_SET);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 2000);
  
  // Format the file system if the lua script hasn't been saved...
//  if (NULL != strstr((char *)&sESP12.rxBuf[30], "cannot open init.lua"))
//  {
//    uint32_t len = sprintf((char *)sESP12.txBuf, "file.format()\r\n");
//    HAL_UART_Transmit(spUART, sESP12.txBuf, len, 1000);
//    HAL_UART_Receive(spUART, sESP12.rxBuf, 14, 5000);  // Get the response
//  }
//  ESP12_setup(false);
}

/**************************************************************************************************\
* FUNCTION    ESP12_init
* DESCRIPTION Initializes the SBT263 module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ESP12_init(UART_HandleTypeDef *pUART)
{
  Util_fillMemory(&sESP12, sizeof(sESP12), 0x00);
  spUART = pUART;
  
  // Reset the device
  ESP12_resetDevice();
  
  ESP12_setPowerProfile(ESP_PROFILE_STANDARD);  // Set all states to 3.3v
  sESP12.state = ESP_STATE_IDLE;
  ESP12_setup(FALSE);
  sESP12.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    ESP12_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the SBT263
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state of the I2C pins
\**************************************************************************************************/
boolean ESP12_setup(boolean state)
{
  if (state)
    HAL_UART_Init(spUART);
  else
    HAL_UART_DeInit(spUART);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    ESP12_getState
* DESCRIPTION Returns the current state of SBT263
* PARAMETERS  None
* RETURNS     The current state of SBT263
\**************************************************************************************************/
ESPState ESP12_getState(void)
{
  return sESP12.state;
}

/**************************************************************************************************\
* FUNCTION    ESP12_getStateAsWord
* DESCRIPTION Returns the current state of SBT263
* PARAMETERS  None
* RETURNS     The current state of SBT263
\**************************************************************************************************/
uint32 ESP12_getStateAsWord(void)
{
  return (uint32)sESP12.state;
}

/**************************************************************************************************\
* FUNCTION    ESP12_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double ESP12_getStateVoltage(void)
{
  return sESP12.vDomain[sESP12.state];
}

/**************************************************************************************************\
* FUNCTION    ESP12_setState
* DESCRIPTION Sets the internal state of SBT263 and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void ESP12_setState(ESPState state)
{
  if (sESP12.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  sESP12.state = state;
  PowerCon_setDeviceDomain(DEVICE_TEMPSENSE, VOLTAGE_DOMAIN_0);
  //Analog_setDomain(SPI_DOMAIN, TRUE, sESP12.vDomain[state]);
}

/**************************************************************************************************\
* FUNCTION    ESP12_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of SBT263
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean ESP12_setPowerState(ESPState state, double vDomain)
{
  if (state >= ESP_STATE_MAX)
    return FALSE;
  else if (vDomain > 5.0)
    return FALSE;
  else
    sESP12.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    ESP12_setPowerProfile
* DESCRIPTION Sets all power states of the temperature/humidity sensor to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean ESP12_setPowerProfile(ESPPowerProfile profile)
{
  uint32 state;
  if (profile >= ESP_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < ESP_STATE_MAX; state++)
    sESP12.vDomain[state] = ESP_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    ESP12_notifyVoltageChange
* DESCRIPTION Called when any other task changes the voltage of the domain
* PARAMETERS  newVoltage: The voltage that the domain is now experiencing
* RETURNS     Nothing
\**************************************************************************************************/
void ESP12_notifyVoltageChange(double newVoltage)
{
  if (newVoltage < 2.3)
    sESP12.state = ESP_STATE_IDLE;
}
/*
void ESP12_sendInitLua(void)
{
  uint32_t len;
  
  len = sprintf((char *)sESP12.txBuf, "file.open(\"init.lua\", \"w+\")");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
  
  len = sprintf((char *)sESP12.txBuf, "file.writeline('wifi.setmode(wifi.STATION)')");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
  
  len = sprintf((char *)sESP12.txBuf, "file.writeline(\"wifi.sta.config(\"SSID\",\"password\")\r\n");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
  
  len = sprintf((char *)sESP12.txBuf, "file.writeline(\"dofile(\"main.lua\")");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
  
  len = sprintf((char *)sESP12.txBuf, "ip, nm, gw=wifi.sta.getip()\r\n");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  
  len = sprintf((char *)sESP12.txBuf, "print(\"\nIP Info:\nIP Address: \"..ip..\" \nNetmask: \"..nm..\" \nGateway Addr: \"..gw..\"\n\")\r\n");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 100);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 5000);

  -- Configure Wireless Internet
  print('\nAll About Circuits init.lua\n')
  wifi.setmode(wifi.STATION)
  print('set mode=STATION (mode='..wifi.getmode()..')\n')
  print('MAC Address: ',wifi.sta.getmac())
  print('Chip ID: ',node.chipid())
  print('Heap Size: ',node.heap(),'\n')
  -- wifi config start
  wifi.sta.config(ssid,pass)
  -- wifi config end

  -- Run the main file
  dofile("main.lua")
}
*/
/**************************************************************************************************\
* FUNCTION    ESP12_test
* DESCRIPTION Interfaces with the SBT263-000 (I2C) temperature andS humidity sensor
* PARAMETERS  measure - sends a measurement command
*             read - reads the temperature and humidity from the SBT263
*             convert - converts the temperature and humidity bit counts into degF and %RH
* RETURNS     Nothing
\**************************************************************************************************/
bool ESP12_test(void)
{
  uint32_t len;
  
  ESP12_setup(true);
  memset(sESP12.txBuf, 0x00, sizeof(sESP12.txBuf));
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 50);
  
  memset(sESP12.txBuf, 0x00, sizeof(sESP12.txBuf));
  len = sprintf((char *)sESP12.txBuf, "majorVer, minorVer, devVer, chipid, flashid, flashsize, \
                                       flashmode, flashspeed = node.info()\r\n");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 50);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
  
  memset(sESP12.txBuf, 0x00, sizeof(sESP12.txBuf));
  len = sprintf((char *)sESP12.txBuf, "print(\"NodeMCU \"..majorVer..\".\"..minorVer..\".\"..devVer)\r\n");
  HAL_UART_Transmit(spUART, sESP12.txBuf, len, 50);
  HAL_UART_Receive(spUART, sESP12.rxBuf, sizeof(sESP12.rxBuf), 500);
    
  ESP12_setup(false);
  return (NULL != strstr((char *)sESP12.rxBuf, "0.9.5"));
}
