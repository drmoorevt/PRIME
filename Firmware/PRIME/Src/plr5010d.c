#include "analog.h"
#include "plr5010d.h"
#include "spi.h"
#include "time.h"

#define _PLR_CLR_Pin GPIO_PIN_12
#define _PLR_CLR_GPIO_Port GPIOB

#define SELECT_ALL_PLR5010D_CLR()                                         \
do {                                                                      \
     HAL_GPIO_WritePin(_PLR_CLR_GPIO_Port, _PLR_CLR_Pin, GPIO_PIN_SET);   \
     Time_delay(1);                                                       \
   } while (0)

#define DESELECT_ALL_PLR5010D_CLR()                                       \
do {                                                                      \
     HAL_GPIO_WritePin(_PLR_CLR_GPIO_Port, _PLR_CLR_Pin, GPIO_PIN_RESET); \
     Time_delay(1);                                                       \
   } while (0)

static struct
{
  SPI_HandleTypeDef spiHandle;
  DMA_HandleTypeDef txHandle;
  DMA_HandleTypeDef rxHandle;
  uint8 txBuffer[32];
  uint8 rxBuffer[32];
  
} sPLR5010D;

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
}

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
* FUNCTION    PLR5010D_setVoltage
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_setVoltage(PLR5010DSelect device, uint16 outBitsA, uint16 outBitsB)
{
  SPI_setup(true, SPI_CLOCK_RATE_05625000);
  
  /******** Write DAC A ********/
  sPLR5010D.txBuffer[0] = 0x00;
  sPLR5010D.txBuffer[1] = outBitsA >> 8;
  sPLR5010D.txBuffer[2] = outBitsA >> 0;
  
  PLR5010D_selectDevice(device, true);
  SPI_write(sPLR5010D.txBuffer, 3);
  PLR5010D_selectDevice(device, false);
  
  /******** Write DAC B ********/
  sPLR5010D.txBuffer[0] = 0x01;
  sPLR5010D.txBuffer[1] = outBitsB >> 8;
  sPLR5010D.txBuffer[2] = outBitsB >> 0;
  
  PLR5010D_selectDevice(device, true);
  SPI_write(sPLR5010D.txBuffer, 3);
  PLR5010D_selectDevice(device, false);
  
  SPI_setup(false, SPI_CLOCK_RATE_05625000);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setVoltage
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool PLR5010D_test(PLR5010DSelect select)
{
  SELECT_ALL_PLR5010D_CLR();
  Time_delay(10000);
  DESELECT_ALL_PLR5010D_CLR();
  Time_delay(10000);
  
  double initialInCurrent, initialOutCurrent;
  double test1InCurrent,   test2OutCurrent;
  
  
  switch (select)
  {
    case PLR5010D_DOMAIN0:
      initialOutCurrent = Analog_getADCVoltage(ADC_DOM0_OUTCURRENT);
      PLR5010D_setVoltage(select,
      break;
    case PLR5010D_DOMAIN1:
      Analog_getADCVoltage(ADC_DOM1_INCURRENT);
      Analog_getADCVoltage(ADC_DOM1_OUTCURRENT);
    
      break;
    case PLR5010D_DOMAIN2:
      Analog_getADCVoltage(ADC_DOM2_INCURRENT);
      Analog_getADCVoltage(ADC_DOM2_OUTCURRENT);
    
      break;
  }
  
  
  
  // reset the device
  // write 0V
  // check current;  -- this is the baseline
  // write 1V
  // check current; -- this is the next current value
  // write 2V
  // check current; --this is the last value;
  return true;
}
