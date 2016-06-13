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
* FUNCTION    PLR5010D_init
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_init(void)
{ 
  return TRUE;
}

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
* FUNCTION    PLR5010D_setVoltage
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_setVoltage(PLR5010DSelect device, PLR5010DChannel chan, double volts)
{
  switch (chan)
  {
    case PLR5010D_CHANA: sPLR5010D.cmd.address = DAC_A; break;
    case PLR5010D_CHANB: sPLR5010D.cmd.address = DAC_B; break;
    default: return false;
  }
  sPLR5010D.cmd.command = WRITE_DACX_INREG_UPDATE_DACX;
  sPLR5010D.cmd.data = (volts/3.3)*65535;
  sPLR5010D.cmd.data = (sPLR5010D.cmd.data >> 8) + (sPLR5010D.cmd.data << 8);
  PLR5010D_spiSend(device, sPLR5010D.txBuf, 3);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setCurrent
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_setCurrent(PLR5010DSelect device, PLR5010DChannel chan, ADCSelect adc, double current)
{
  double loCurrent  = current * .95;
  double hiCurrent  = current * 1.05;
  double maxCurrent = current * 1.25;
  double i, initCurrent, actCurrent;
  
  PLR5010D_setVoltage(device, chan, 0.0);
  HAL_Delay(1);
  initCurrent = Analog_getADCCurrent(adc);
  loCurrent  += initCurrent;
  hiCurrent  += initCurrent;
  maxCurrent += initCurrent;
  
  for (i = 0; i < 3.3; i += .010)
  {
    actCurrent = Analog_getADCCurrent(adc);
    PLR5010D_setVoltage(device, chan, i);
    if ((actCurrent > loCurrent) && (actCurrent < hiCurrent))
      return true;
    if (actCurrent > maxCurrent)
      return false;
  }
  return false;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setVoltage
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
    
    initialOutCurrent = Analog_getADCCurrent(outCurrentADC);
    PLR5010D_setCurrent(device, PLR5010D_CHANA, outCurrentADC, 50);
    PLR5010D_setCurrent(device, PLR5010D_CHANB, outCurrentADC, 50);
    HAL_Delay(1);
    testOutCurrent = Analog_getADCCurrent(outCurrentADC);
    
    PLR5010D_clearAllDevices();
    
    iDiff = (testOutCurrent - initialOutCurrent);
   if ((iDiff > 80) && (iDiff < 120))
     return true;
  }
  return false;
}
