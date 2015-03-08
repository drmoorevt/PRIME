
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

static struct
{
  ADC_HandleTypeDef    adcHandle[ANALOG_HANDLE_ADC_MAX];
} sAnalog;

static void Error_Handler(void)
{
  while(1)
  {
  }
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

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  static DMA_HandleTypeDef  hdma_adc;

  ADCx_CHANNEL_GPIO_CLK_ENABLE();/* Enable GPIO clock */
  ADCx_CLK_ENABLE();/* ADC3 Periph clock enable */
  DMAx_CLK_ENABLE(); /* Enable DMA2 clock */

  /* ADC3 Channel5 GPIO pin configuration */
  GPIO_InitStruct.Pin  = ADCx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ADCx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);

  /*##-3- Configure the DMA streams ##########################################*/
  /* Set the parameters to be configured */
  hdma_adc.Instance = ADCx_DMA_STREAM;

  hdma_adc.Init.Channel  = ADCx_DMA_CHANNEL;
  hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc.Init.Mode = DMA_PFCTRL;
  hdma_adc.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_adc.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_adc.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_adc.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_adc.Init.PeriphBurst = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdma_adc);

  /* Associate the initialized DMA handle to the the ADC handle */
  __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc);

  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(ADCx_DMA_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADCx_DMA_IRQn);
}

// setup ADC
boolean Main_setupADC(AnalogAdcHandle handle)
{
  ADC_ChannelConfTypeDef chanConfig;
  /*##-1- Configure the ADC peripheral #######################################*/
  sAnalog.adcHandle[handle].Instance = pAdc;

  sAnalog.adcHandle[handle].Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
  sAnalog.adcHandle[handle].Init.Resolution            = ADC_RESOLUTION_12B;
  sAnalog.adcHandle[handle].Init.ScanConvMode          = DISABLE;
  sAnalog.adcHandle[handle].Init.ContinuousConvMode    = ENABLE;
  sAnalog.adcHandle[handle].Init.DiscontinuousConvMode = DISABLE;
  sAnalog.adcHandle[handle].Init.NbrOfDiscConversion   = 0;
  sAnalog.adcHandle[handle].Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  sAnalog.adcHandle[handle].Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_CC1;
  sAnalog.adcHandle[handle].Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  sAnalog.adcHandle[handle].Init.NbrOfConversion       = 1;
  sAnalog.adcHandle[handle].Init.DMAContinuousRequests = ENABLE;
  sAnalog.adcHandle[handle].Init.EOCSelection          = DISABLE;

  // Turn on all the clocks ...
  (RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN));
  (RCC->APB2ENR |= (RCC_APB2ENR_ADC2EN));
  (RCC->APB2ENR |= (RCC_APB2ENR_ADC3EN));

  if(HAL_ADC_Init(&sAnalog.adcHandle[handle]) != HAL_OK)
    Error_Handler(); /* Initialization Error */

  /*##-2- Configure ADC regular channel ######################################*/
  switch (handle)
  {
    case ANALOG_HANDLE_ADC_1: chanConfig.Channel = ADC_CHANNEL_4; break;
    case ANALOG_HANDLE_ADC_2: chanConfig.Channel = ADC_CHANNEL_5; break;
    case ANALOG_HANDLE_ADC_3: chanConfig.Channel = ADC_CHANNEL_5; break;
  }
  chanConfig.Rank = 1;
  chanConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  chanConfig.Offset = 0;

  if(HAL_ADC_ConfigChannel(&sAnalog.adcHandle[handle], &chanConfig) != HAL_OK)
    Error_Handler(); /* Channel Configuration Error */

  return TRUE;
}

// setup DMA
boolean Main_setupDMA(void)
{
  static DMA_HandleTypeDef dmaHandle1, dmaHandle2, dmaHandle3;
  DMA_InitTypeDef init1, init2, init3 =
  { .Channel             = DMA_CHANNEL_0,
    .Direction           = DMA_PERIPH_TO_MEMORY,
    .PeriphInc           = DMA_PINC_DISABLE,
    .MemInc              = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
    .MemDataAlignment    = DMA_MDATAALIGN_HALFWORD,
    .Priority            = DMA_PRIORITY_MEDIUM,
    .FIFOMode            = DMA_FIFOMODE_ENABLE,
    .FIFOThreshold       = DMA_FIFO_THRESHOLD_1QUARTERFULL,
    .MemBurst            = DMA_MBURST_SINGLE,
    .PeriphBurst         = DMA_PBURST_SINGLE
  };
  init1.Channel = DMA_CHANNEL_0;
  init2.Channel = DMA_CHANNEL_1;
  init3.Channel = DMA_CHANNEL_2;

  dmaHandle1.Init = init1;
  dmaHandle1.Instance = DMA2_Stream0;
  dmaHandle1.XferCpltCallback = dmaTransferComplete;
  dmaHandle1.XferErrorCallback = dmaTransferComplete;
  dmaHandle1.XferHalfCpltCallback = dmaTransferComplete;
  dmaHandle1.XferM1CpltCallback = dmaTransferComplete;

  dmaHandle2.Init = init2;
  dmaHandle2.Instance = DMA2_Stream2;
  dmaHandle2.XferCpltCallback = dmaTransferComplete;
  dmaHandle2.XferErrorCallback = dmaTransferComplete;
  dmaHandle2.XferHalfCpltCallback = dmaTransferComplete;
  dmaHandle2.XferM1CpltCallback = dmaTransferComplete;

  dmaHandle3.Init = init3;
  dmaHandle3.Instance = DMA2_Stream1;
  dmaHandle3.XferCpltCallback = dmaTransferComplete;
  dmaHandle3.XferErrorCallback = dmaTransferComplete;
  dmaHandle3.XferHalfCpltCallback = dmaTransferComplete;
  dmaHandle3.XferM1CpltCallback = dmaTransferComplete;

  HAL_DMA_Init(&dmaHandle1);
  HAL_DMA_Init(&dmaHandle2);
  HAL_DMA_Init(&dmaHandle3);

  return TRUE;
}

// setup ADC
boolean Main_setupTimer(void)
{

  return TRUE;
}

uint32 samples[100];

boolean Analog_testAnalogBandwidth(void)
{
  Main_setupADC(ADC1);
  Main_setupADC(ADC2);
  Main_setupADC(ADC3);
  Main_setupTimer();

  if(HAL_ADC_Start_DMA(&sAnalog.adcHandle[handle], (uint32*)SDRAM_DEVICE_ADDR, SDRAM_DEVICE_SIZE) != HAL_OK)
  {
    /* Start Conversation Error */
    Error_Handler();
  }

  return TRUE;
}
