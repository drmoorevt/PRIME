#include "analog.h"
#include "plr5010d.h"
#include "spi.h"
#include "time.h"
#include "util.h"

#define _PLR_CLR_Pin GPIO_PIN_12
#define _PLR_CLR_GPIO_Port GPIOB

#define SELECT_ALL_PLR5010D_CLR()                                         \
do {                                                                      \
     HAL_GPIO_WritePin(_PLR_CLR_GPIO_Port, _PLR_CLR_Pin, GPIO_PIN_RESET); \
     Time_delay(1);                                                       \
   } while (0)

#define DESELECT_ALL_PLR5010D_CLR()                                       \
do {                                                                      \
     HAL_GPIO_WritePin(_PLR_CLR_GPIO_Port, _PLR_CLR_Pin, GPIO_PIN_SET);   \
     Time_delay(1);                                                       \
   } while (0)

typedef enum
{
  WRITE_DACX_INREG             = 0x0,
  UPDATE_DACX                  = 0x1,
  WRITE_DACX_INREG_UPDATE_ALL  = 0x2,
  WRITE_DACX_INREG_UPDATE_DACX = 0x3,
  POWER_CONTROL_DACX           = 0x4,
  POWER_ON_RESET               = 0x5,
  LDAC_CONTROL                 = 0x6,
  REFERENCE_CONTROL            = 0x7,
} DAC856XCmd;

typedef enum
{
  DAC_A   = 0x0,
  DAC_B   = 0x1,
  DAC_ALL = 0x7,
} DAC856XAddr;

typedef struct
{
  uint32_t address  : 3;
  uint32_t command  : 3;
  uint32_t dontCare : 2;
  uint32_t data     : 16;
  uint32_t dummy    : 8;
} DAC856XCommand;

#pragma anon_unions

static struct
{
  union
  {
    DAC856XCommand cmd;
    uint8_t txBuf[3];
  };
} sPLR5010D;

/**************************************************************************************************\
* FUNCTION    PLR5010D_clearAllDevices
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
void PLR5010D_clearAllDevices(void)
{
  SELECT_ALL_PLR5010D_CLR();
  Time_delay(1000);
  DESELECT_ALL_PLR5010D_CLR();
  Time_delay(1000);
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_selectDevice
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
void PLR5010D_selectDevice(PLR5010DSelect device, bool select)
{
  GPIO_PinState pinState = (select) ? GPIO_PIN_RESET : GPIO_PIN_SET;
  switch (device)
  {
    case PLR5010D_DOMAIN0: HAL_GPIO_WritePin(PLR0_CS_GPIO_Port, PLR0_CS_Pin, pinState); break;
    case PLR5010D_DOMAIN1: HAL_GPIO_WritePin(PLR1_CS_GPIO_Port, PLR1_CS_Pin, pinState); break;
    case PLR5010D_DOMAIN2: HAL_GPIO_WritePin(PLR2_CS_GPIO_Port, PLR2_CS_Pin, pinState); break;
    default: break;
  }
  Time_delay(1);
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_spiSend
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
void PLR5010D_spiSend(PLR5010DSelect device, uint8_t *pBytes, uint32_t len)
{
  SPI_setup(true, SPI_CLOCK_RATE_05625000, SPI_PHASE_1EDGE, SPI_POLARITY_HIGH, SPI_MODE_NORMAL);
  PLR5010D_selectDevice(device, true);
  SPI_write(pBytes, len);
  PLR5010D_selectDevice(device, false);
  SPI_setup(false, SPI_CLOCK_RATE_05625000, SPI_PHASE_1EDGE, SPI_POLARITY_HIGH, SPI_MODE_NORMAL);
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setConfig
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_setConfig(PLR5010DSelect device)
{
  sPLR5010D.cmd.command = REFERENCE_CONTROL;  // Make sure the external reference is used
  sPLR5010D.cmd.address = 0;
  sPLR5010D.cmd.data = 0;
  PLR5010D_spiSend(device, sPLR5010D.txBuf, 3);
  
  sPLR5010D.cmd.command = WRITE_DACX_INREG;  // Make sure the gain is 1
  sPLR5010D.cmd.address = 2;
  sPLR5010D.cmd.data = (3 << 8);
  PLR5010D_spiSend(device, sPLR5010D.txBuf, 3);
  return true;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_init
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_init(void)
{
  PLR5010D_clearAllDevices();
  PLR5010D_setConfig(PLR5010D_DOMAIN0);
  PLR5010D_setConfig(PLR5010D_DOMAIN1);
  PLR5010D_setConfig(PLR5010D_DOMAIN2);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setVoltage
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_setVoltage(PLR5010DSelect device, PLR5010DChannel chan, uint16_t voltCode)
{
  switch (chan)
  {
    case PLR5010D_CHANA:     sPLR5010D.cmd.address = DAC_A;   break;
    case PLR5010D_CHANB:     sPLR5010D.cmd.address = DAC_B;   break;
    case PLR5010D_CHAN_BOTH: sPLR5010D.cmd.address = DAC_ALL; break;
    default: return false;
  }
  sPLR5010D.cmd.command = WRITE_DACX_INREG_UPDATE_DACX;
  sPLR5010D.cmd.data = (voltCode >> 8) + (voltCode << 8);
  PLR5010D_spiSend(device, sPLR5010D.txBuf, 3);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setCurrent
* DESCRIPTION Sets the current sink of the PLR5010D in mA
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Linearization: I = [.001733*(Vcc - 1.8)*Vbjt + .0449] - [.0151 - (Vcc - 1.8)*.004867]
*             Therefore   Vbjt = [[.0151 - (Vcc - 1.8)*.004867] - .0449] / [.001733*(Vcc - 1.8)]
*                              = (current + .0151 - .004867*(Vcc - 1.8)) / (.001733*(Vcc - 1.8) + .0449)
\**************************************************************************************************/
bool PLR5010D_setCurrent(PLR5010DSelect device, PLR5010DChannel chan, ADCSelect adc, double current)
{ 
  #define MAX_CURRENT  (275.0)
  #define EXPECTED_ERROR (0.050)  // Allow 50uA from ideal current
  double domVoltage, refVoltage, bjtVoltage, initCurrent, actCurrent;
  PLR5010D_setVoltage(device, chan, 0);  // Turn off this channel to capture initialCurrent
  initCurrent = Analog_getADCCurrent(adc, 10);
  if (current < EXPECTED_ERROR)
    return true;
  double loCurrent = MAX(current - EXPECTED_ERROR,           0) + initCurrent;
  double hiCurrent = MIN(current + EXPECTED_ERROR, MAX_CURRENT) + initCurrent;
  if (PLR5010D_CHAN_BOTH == chan)  // If we're setting both channels, double the expected current
  {
    loCurrent *= 2;
    hiCurrent *= 2;
  }
  
  // Start with the best guess based on linearization
  switch (device)
  {
    case PLR5010D_DOMAIN0: domVoltage = Analog_getADCVoltage(ADC_DOM0_VOLTAGE, 10); break;
    case PLR5010D_DOMAIN1: domVoltage = Analog_getADCVoltage(ADC_DOM1_VOLTAGE, 10); break;
    case PLR5010D_DOMAIN2: domVoltage = Analog_getADCVoltage(ADC_DOM2_VOLTAGE, 10); break;
    default: return false;
  }
  refVoltage = Analog_getADCVoltage(ADC_DOM0_VOLTAGE, 10);
  bjtVoltage = (current/1000 + .0151 - .004867*(domVoltage - 1.8)) / 
                      (.001733*(domVoltage - 1.8) + .0449);
  
  do
  {
    PLR5010D_setVoltage(device, chan, MIN((uint16_t)(65535.0 * bjtVoltage / refVoltage), 65535));
    actCurrent = Analog_getADCCurrent(adc, 10);
    if (actCurrent > hiCurrent)
    {  // Actual current is more than requested current, lower bjtVoltage by the % difference
      bjtVoltage = bjtVoltage * (loCurrent / actCurrent);
      //bjtVoltage = bjtVoltage - 10/65535.0;
    }
    else if (actCurrent < loCurrent)
    {  // Actual current is less than requested current, raise bjtVoltage by the % difference
      bjtVoltage = bjtVoltage * (hiCurrent / actCurrent);
      //bjtVoltage = bjtVoltage + 10/65535.0;
    }
  } while ((actCurrent > hiCurrent) || (actCurrent < loCurrent));
  
  return true;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_test
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_test(PLR5010DSelect device)
{
  double initialOutCurrent, testOutCurrent, iDiff;
  ADCSelect outCurrentADC;
  uint32_t i;
  
  for (i = 0; i < 10; i++)
  {
  
    PLR5010D_clearAllDevices();
    
    sPLR5010D.cmd.command = LDAC_CONTROL;
    sPLR5010D.cmd.address = DAC_A;
    sPLR5010D.cmd.data = 3;
    sPLR5010D.cmd.data = (sPLR5010D.cmd.data >> 8) + (sPLR5010D.cmd.data << 8);
    PLR5010D_spiSend(device, sPLR5010D.txBuf, 3);
    Time_delay(1000);
    
    switch (device)
    {
      case PLR5010D_DOMAIN0: outCurrentADC = ADC_DOM0_OUTCURRENT; break;
      case PLR5010D_DOMAIN1: outCurrentADC = ADC_DOM1_OUTCURRENT; break;
      case PLR5010D_DOMAIN2: outCurrentADC = ADC_DOM2_OUTCURRENT; break;
      default: return false;
    }
    
    initialOutCurrent = Analog_getADCCurrent(outCurrentADC, 10);
    PLR5010D_setCurrent(device, PLR5010D_CHANA, outCurrentADC, 50);
    PLR5010D_setCurrent(device, PLR5010D_CHANB, outCurrentADC, 50);
    HAL_Delay(1);
    testOutCurrent = Analog_getADCCurrent(outCurrentADC, 10);
    
    PLR5010D_clearAllDevices();
    
    iDiff = (testOutCurrent - initialOutCurrent);
   if ((iDiff > 80) && (iDiff < 120))
     return true;
  }
  return false;
}
