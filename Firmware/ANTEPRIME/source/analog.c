#include "analog.h"
#include "extmem.h"
#include "stm32f4xx_hal.h"

typedef enum
{
  ANALOG_HANDLE_ADC_1   = 0,
  ANALOG_HANDLE_ADC_2   = 1,
  ANALOG_HANDLE_ADC_3   = 2,
  ANALOG_HANDLE_ADC_MAX = 3,
} AnalogAdcHandle;

typedef enum
{
  ANALOG_HANDLE_DAC_1   = 0,
  ANALOG_HANDLE_DAC_2   = 1,
  ANALOG_HANDLE_DAC_MAX = 3,
} AnalogDacHandle;

static struct
{
  struct
  {
    ADC_HandleTypeDef    adcHandle[ANALOG_HANDLE_ADC_MAX];
    DMA_HandleTypeDef    dmaHandle[ANALOG_HANDLE_ADC_MAX];
    TIM_HandleTypeDef    timHandle;
  } adc;
  struct
  {
    DAC_HandleTypeDef    dacHandle[ANALOG_HANDLE_DAC_MAX];
  } dac;
} sAnalog;

/**************************************************************************************************\
* FUNCTION    Error_Handler
* DESCRIPTION Generic error handler for use when intended callbacks dont exist
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static void Error_Handler(void)
{
  while(1) { }
}

/**************************************************************************************************\
* FUNCTION    DMA2_Stream0_IRQHandler
* DESCRIPTION Catches interrupts from the DMA stream and dispatches them to their handler
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       DMA2, Stream0 is configured for DMA via ADC_1 (DMA Channel 0, ADC Channel 14, pin C4)
\**************************************************************************************************/
void DMA2_Stream0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(sAnalog.adc.adcHandle[ANALOG_HANDLE_ADC_1].DMA_Handle);
  return;
}

/**************************************************************************************************\
* FUNCTION    DMA2_Stream1_IRQHandler
* DESCRIPTION Catches interrupts from the DMA stream and dispatches them to their handler
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       DMA2, Stream1 is configured for DMA via ADC_3 (DMA Channel 2, ADC Channel 11, pin C1)
\**************************************************************************************************/
void DMA2_Stream1_IRQHandler(void)
{
  return;
}

/**************************************************************************************************\
* FUNCTION    DMA2_Stream2_IRQHandler
* DESCRIPTION Catches interrupts from the DMA stream and dispatches them to their handler
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       DMA2, Stream2 is configured for DMA via ADC_2 (DMA Channel 1, ADC Channel 15, pin C5)
\**************************************************************************************************/
void DMA2_Stream2_IRQHandler(void)
{
  return;
}

void dmaTransferComplete(struct __DMA_HandleTypeDef * hdma)
{
  
}
void dmaTransferHalfComplete(struct __DMA_HandleTypeDef * hdma)
{
  
}
void dmaTransferM1Complete(struct __DMA_HandleTypeDef * hdma)
{
  
}
void dmaTransferError(struct __DMA_HandleTypeDef * hdma)
{
  
}

/**************************************************************************************************\
* FUNCTION    Analog_dmaInit
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Analog_dmaInit(AnalogAdcHandle handle)
{
  DMAx_CLK_ENABLE(); /* Enable DMA2 clock */
  /*##-3- Configure the DMA streams ##########################################*/
  switch (handle)
  {
    case ANALOG_HANDLE_ADC_1:
      sAnalog.adc.dmaHandle[handle].Instance             = DMA2_Stream0;
      sAnalog.adc.dmaHandle[handle].Init.Channel         = DMA_CHANNEL_0;
      break;
    case ANALOG_HANDLE_ADC_2:
      sAnalog.adc.dmaHandle[handle].Instance             = DMA2_Stream2;
      sAnalog.adc.dmaHandle[handle].Init.Channel         = DMA_CHANNEL_1;
      break;
    case ANALOG_HANDLE_ADC_3:
      sAnalog.adc.dmaHandle[handle].Instance             = DMA2_Stream1;
      sAnalog.adc.dmaHandle[handle].Init.Channel         = DMA_CHANNEL_2;
      break;
  }
  sAnalog.adc.dmaHandle[handle].Init.Direction           = DMA_PERIPH_TO_MEMORY;
  sAnalog.adc.dmaHandle[handle].Init.PeriphInc           = DMA_PINC_DISABLE;
  sAnalog.adc.dmaHandle[handle].Init.MemInc              = DMA_MINC_ENABLE;
  sAnalog.adc.dmaHandle[handle].Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  sAnalog.adc.dmaHandle[handle].Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  sAnalog.adc.dmaHandle[handle].Init.Mode                = DMA_NORMAL;
  sAnalog.adc.dmaHandle[handle].Init.Priority            = DMA_PRIORITY_HIGH;
  sAnalog.adc.dmaHandle[handle].Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
  sAnalog.adc.dmaHandle[handle].Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
  sAnalog.adc.dmaHandle[handle].Init.MemBurst            = DMA_MBURST_SINGLE;
  sAnalog.adc.dmaHandle[handle].Init.PeriphBurst         = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&sAnalog.adc.dmaHandle[handle]);

  /* Associate the initialized DMA handle to the the ADC handle */
  __HAL_LINKDMA(&sAnalog.adc.adcHandle[handle], DMA_Handle, sAnalog.adc.dmaHandle[handle]);

  /*##-4- Configure the NVIC for DMA #########################################*/
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**************************************************************************************************\
* FUNCTION    Analog_setupADC
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean Analog_setupADC(AnalogAdcHandle handle)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  ADCx_CHANNEL_GPIO_CLK_ENABLE();/* Enable GPIO clock */
  /* ADC3 Channel5 GPIO pin configuration */
  GPIO_InitStruct.Pin  = ADCx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  ADC_ChannelConfTypeDef chanConfig;
  /*##-1- Configure the ADC peripheral #######################################*/
  sAnalog.adc.adcHandle[handle].Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
  sAnalog.adc.adcHandle[handle].Init.Resolution            = ADC_RESOLUTION_12B;
  sAnalog.adc.adcHandle[handle].Init.ScanConvMode          = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.ContinuousConvMode    = ENABLE;
  sAnalog.adc.adcHandle[handle].Init.DiscontinuousConvMode = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.NbrOfDiscConversion   = 0;
  sAnalog.adc.adcHandle[handle].Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  sAnalog.adc.adcHandle[handle].Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_CC1;
  sAnalog.adc.adcHandle[handle].Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  sAnalog.adc.adcHandle[handle].Init.NbrOfConversion       = 1;
  sAnalog.adc.adcHandle[handle].Init.DMAContinuousRequests = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.EOCSelection          = DISABLE;

  /*##-2- Configure ADC regular channel ######################################*/
  switch (handle)
  {
    case ANALOG_HANDLE_ADC_1:
      sAnalog.adc.adcHandle[handle].Instance = ADC1;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN));
      chanConfig.Channel = ADC_CHANNEL_14; // GPIO_PIN_4 (PortC)
      break;
    case ANALOG_HANDLE_ADC_2:
      sAnalog.adc.adcHandle[handle].Instance = ADC2;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC2EN));
      chanConfig.Channel = ADC_CHANNEL_15; // GPIO_PIN_5 (PortC)
      break;
    case ANALOG_HANDLE_ADC_3:
      sAnalog.adc.adcHandle[handle].Instance = ADC3;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC3EN));
      chanConfig.Channel = ADC_CHANNEL_11; // GPIO_PIN_1 (PortC)
      break;
    default:
      break;
  }
  chanConfig.Rank = 1;
  chanConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  chanConfig.Offset = 0;

  Analog_dmaInit(handle);

  if(HAL_ADC_Init(&sAnalog.adc.adcHandle[handle]) != HAL_OK)
    Error_Handler(); /* Initialization Error */

  if(HAL_ADC_ConfigChannel(&sAnalog.adc.adcHandle[handle], &chanConfig) != HAL_OK)
    Error_Handler(); /* Channel Configuration Error */

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    Analog_setupTimer
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean Analog_setupTimer(void)
{
  sAnalog.adc.timHandle.Instance = TIM2;
  sAnalog.adc.timHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  sAnalog.adc.timHandle.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  sAnalog.adc.timHandle.Init.Period = 1;
  sAnalog.adc.timHandle.Init.Prescaler = 180; // yields 1us per tick
  sAnalog.adc.timHandle.Init.RepetitionCounter = 1;
  sAnalog.adc.timHandle.Channel = HAL_TIM_ACTIVE_CHANNEL_1;
  TIM_Base_SetConfig(sAnalog.adc.timHandle.Instance, &sAnalog.adc.timHandle.Init);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    Analog_testAnalogBandwidth
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       This function configures the ADCs to DMA the results of 0x10 continuous transfers to
              predefined locations in memory.
\**************************************************************************************************/
boolean Analog_testAnalogBandwidth(void)
{
  uint32 ADC_NBR_SAMPLES   = 0x10;
  uint16 *ADC1_SAMPLE_ADDR = (uint16 *)(SDRAM_DEVICE_ADDR);
  uint16 *ADC2_SAMPLE_ADDR = (uint16 *)(ADC1_SAMPLE_ADDR + ADC_NBR_SAMPLES);
  uint16 *ADC3_SAMPLE_ADDR = (uint16 *)(ADC2_SAMPLE_ADDR + ADC_NBR_SAMPLES);

  Analog_setupADC(ANALOG_HANDLE_ADC_1);
  Analog_setupADC(ANALOG_HANDLE_ADC_2);
  Analog_setupADC(ANALOG_HANDLE_ADC_3);
//  Analog_setupTimer();

  if(HAL_ADC_Start_DMA(&sAnalog.adc.adcHandle[ANALOG_HANDLE_ADC_1], (uint32 *)ADC1_SAMPLE_ADDR, ADC_NBR_SAMPLES) != HAL_OK)
    Error_Handler();
  if(HAL_ADC_Start_DMA(&sAnalog.adc.adcHandle[ANALOG_HANDLE_ADC_2], (uint32 *)ADC2_SAMPLE_ADDR, ADC_NBR_SAMPLES) != HAL_OK)
    Error_Handler();
  if(HAL_ADC_Start_DMA(&sAnalog.adc.adcHandle[ANALOG_HANDLE_ADC_3], (uint32 *)ADC3_SAMPLE_ADDR, ADC_NBR_SAMPLES) != HAL_OK)
    Error_Handler();

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    Analog_setupDAC
* DESCRIPTION Sets up the DAC to output analog voltages
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean Analog_setupDAC(AnalogDacHandle handle)
{
  
}

/**************************************************************************************************\
* FUNCTION    Analog_testAnalogBandwidth
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       This function configures the ADCs to DMA the results of 0x10 continuous transfers to
              predefined locations in memory.
\**************************************************************************************************/
boolean Analog_testDAC(void)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  ADCx_CHANNEL_GPIO_CLK_ENABLE();/* Enable GPIO clock */
  /* ADC3 Channel5 GPIO pin configuration */
  GPIO_InitStruct.Pin  = ADCx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  ADC_ChannelConfTypeDef chanConfig;
  /*##-1- Configure the ADC peripheral #######################################*/
  sAnalog.adc.adcHandle[handle].Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
  sAnalog.adc.adcHandle[handle].Init.Resolution            = ADC_RESOLUTION_12B;
  sAnalog.adc.adcHandle[handle].Init.ScanConvMode          = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.ContinuousConvMode    = ENABLE;
  sAnalog.adc.adcHandle[handle].Init.DiscontinuousConvMode = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.NbrOfDiscConversion   = 0;
  sAnalog.adc.adcHandle[handle].Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  sAnalog.adc.adcHandle[handle].Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_CC1;
  sAnalog.adc.adcHandle[handle].Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  sAnalog.adc.adcHandle[handle].Init.NbrOfConversion       = 1;
  sAnalog.adc.adcHandle[handle].Init.DMAContinuousRequests = DISABLE;
  sAnalog.adc.adcHandle[handle].Init.EOCSelection          = DISABLE;

  /*##-2- Configure ADC regular channel ######################################*/
  switch (handle)
  {
    case ANALOG_HANDLE_ADC_1:
      sAnalog.adc.adcHandle[handle].Instance = ADC1;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN));
      chanConfig.Channel = ADC_CHANNEL_14; // GPIO_PIN_4 (PortC)
      break;
    case ANALOG_HANDLE_ADC_2:
      sAnalog.adc.adcHandle[handle].Instance = ADC2;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC2EN));
      chanConfig.Channel = ADC_CHANNEL_15; // GPIO_PIN_5 (PortC)
      break;
    case ANALOG_HANDLE_ADC_3:
      sAnalog.adc.adcHandle[handle].Instance = ADC3;
      (RCC->APB2ENR |= (RCC_APB2ENR_ADC3EN));
      chanConfig.Channel = ADC_CHANNEL_11; // GPIO_PIN_1 (PortC)
      break;
    default:
      break;
  }
  chanConfig.Rank = 1;
  chanConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  chanConfig.Offset = 0;

  Analog_dmaInit(handle);

  if(HAL_ADC_Init(&sAnalog.adc.adcHandle[handle]) != HAL_OK)
    Error_Handler(); /* Initialization Error */

  if(HAL_ADC_ConfigChannel(&sAnalog.adc.adcHandle[handle], &chanConfig) != HAL_OK)
    Error_Handler(); /* Channel Configuration Error */

  return TRUE;
}