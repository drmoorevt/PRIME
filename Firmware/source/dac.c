#include "stm32f2xx.h"
#include "dac.h"
#include "gpio.h"
#include "util.h"

#define FILE_ID DAC_C

/*****************************************************************************\
* FUNCTION    DAC_init
* DESCRIPTION Initializes the DAC pins to their default configurations
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
void DAC_init(void)
{ 
  const uint16 ctrlPortA = (GPIO_Pin_4 | GPIO_Pin_5);
  // Initialize both DAC pins to analog configuration
  GPIO_InitTypeDef analogCtrlPortA = {ctrlPortA, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOA, TRUE);
  GPIO_configurePins(GPIOA, &analogCtrlPortA);

  RCC->APB1ENR |= RCC_APB1ENR_DACEN;        // Enable DAC Clocks
  DAC->CR |= (DAC_CR_EN1 | DAC_CR_EN2);     // Enable both DACs
  DAC->CR |= (DAC_CR_BOFF1 | DAC_CR_BOFF2); // Enable both output buffers
  DAC->CR |= (DAC_CR_TSEL1 | DAC_CR_TSEL2); // Both DACs use software triggers
}

/*****************************************************************************\
* FUNCTION    DAC_setDAC
* DESCRIPTION Sends a 12bit value out over the selected DAC port
* PARAMETERS  port: The DAC port over which to send the value
              outVal: The value to send over the selected port
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
void DAC_setDAC(DACPort port, uint16 outVal)
{
  if (DAC_PORT1 == port)
  {
    DAC->DHR12R1 = (outVal & 0x0FFF);
    DAC->SR |= DAC_SWTRIGR_SWTRIG1;
  }
  else if (DAC_PORT2 == port)
  {
    DAC->DHR12R2 = (outVal & 0x0FFF);
    DAC->SR |= DAC_SWTRIGR_SWTRIG2;
  }
  Util_spinWait(15);
}

/*****************************************************************************\
* FUNCTION    DAC_setDAC
* DESCRIPTION Sends a 12bit value out over the selected DAC port
* PARAMETERS  port: The DAC port over which to send the value
              outVal: The value to send over the selected port
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
void DAC_setVoltage(DACPort port, float outVolts)
{
  uint16 outVal = (uint16)(outVolts * 4095.0 / 3.3);
  DAC_setDAC(port, outVal);
}
