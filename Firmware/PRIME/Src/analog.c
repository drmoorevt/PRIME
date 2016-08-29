#include "analog.h"
#include "tests.h"
#include "util.h"
#include <string.h>

#define FILE_ID ANALOG_C

typedef enum
{
  ANALOG_HANDLE_DAC_1   = 0,
  ANALOG_HANDLE_DAC_2   = 1,
  ANALOG_HANDLE_DAC_MAX = 2,
} AnalogDacHandle;

typedef struct
{
  ADC_HandleTypeDef hadc;
  DMA_HandleTypeDef hdma;
  AppADCConfig      cfg;
} ADCPortCfg;

static struct
{
  ADCPortCfg adc[NUM_ADC_PORTS];
  TIM_HandleTypeDef htim;
} sAnalog;

void Analog_initADC(ADCSelect adcNum);
ADCPort Analog_getPortNumber(ADCSelect adcSelect);
uint32_t Analog_getChannelNumber(ADCSelect adcSelect);

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
* FUNCTION    Analog_init
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool Analog_init(void)
{
  memset(&sAnalog, 0x00, sizeof(sAnalog));
  return true;
}

ADCPortCfg *Analog_getPortCfg(ADC_HandleTypeDef *hadc)
{ 
  uint32_t i;
  for (i = 0; i < NUM_ADC_PORTS; i++)
  {
    if (hadc == &sAnalog.adc[i].hadc)
      return &sAnalog.adc[i];
  }
  return NULL;
}

/**************************************************************************************************\
* FUNCTION    Analog_xferComplete
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  ADCPortCfg *portCfg = Analog_getPortCfg(hadc);
  // Subtract off the number we just completed, and calculate the number of remaining samples
  uint32_t numCompleted = MIN(0xFFFF, portCfg->cfg.adcConfig.numSamps);
  portCfg->cfg.adcConfig.numSamps -= numCompleted;
  uint32_t numToFetch   = MIN(0xFFFF, portCfg->cfg.adcConfig.numSamps);
  // Increment the destination pointer by the number of completed samples
  portCfg->cfg.appSampleBuffer += numCompleted;
  
  if (numToFetch > 0)
  {
    HAL_ADC_Stop_DMA(hadc);
    if(HAL_ADC_Start_DMA(hadc, (uint32 *)portCfg->cfg.appSampleBuffer, numToFetch) != HAL_OK)
      Error_Handler();
  }
  else
  {
    ADCPort port = ADC_PORT1;
    if (ADC1 == hadc->Instance)
      port = ADC_PORT1;
    else if (ADC2 == hadc->Instance)
      port = ADC_PORT2;
    else if (ADC3 == hadc->Instance)
      port = ADC_PORT3;
    Tests_notifyConversionComplete(port, sAnalog.adc[port].cfg.adcConfig.chan[0].chanNum,
                                         sAnalog.adc[port].cfg.adcConfig.numSamps);
  }
}

/**************************************************************************************************\
* FUNCTION    Analog_dmaInit
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Analog_dmaInit(ADCPort adcNum)
{
  IRQn_Type dmaIRQ;
  ADCPortCfg *pADC = &sAnalog.adc[adcNum];
  
  __DMA2_CLK_ENABLE();
  /*##-3- Configure the DMA streams ##########################################*/
  if (NULL != pADC->hdma.Instance)  // Only preform the DeInit if it was previously init
    HAL_DMA_DeInit(&pADC->hdma);
  switch (adcNum)  // Configure the DMA streams 
  {
    case ADC_PORT1:
      pADC->hdma.Instance     = DMA2_Stream0;
      pADC->hdma.Init.Channel = DMA_CHANNEL_0;
      dmaIRQ = DMA2_Stream0_IRQn;
      break;
    case ADC_PORT2:
      pADC->hdma.Instance     = DMA2_Stream2;
      pADC->hdma.Init.Channel = DMA_CHANNEL_1;
      dmaIRQ = DMA2_Stream2_IRQn;
      break;
    case ADC_PORT3:
      pADC->hdma.Instance     = DMA2_Stream1;
      pADC->hdma.Init.Channel = DMA_CHANNEL_2;
      dmaIRQ = DMA2_Stream1_IRQn;
      break;
    default: return;
  }
  pADC->hdma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  pADC->hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
  pADC->hdma.Init.MemInc              = DMA_MINC_ENABLE;
  pADC->hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  pADC->hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  pADC->hdma.Init.Mode                = DMA_NORMAL;
  pADC->hdma.Init.Priority            = DMA_PRIORITY_HIGH;
  pADC->hdma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
  pADC->hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_1QUARTERFULL;
  pADC->hdma.Init.MemBurst            = DMA_MBURST_SINGLE;
  pADC->hdma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
  HAL_DMA_Init(&pADC->hdma);

  /* Associate the initialized DMA handle to the the ADC handle */
  __HAL_LINKDMA(&pADC->hadc, DMA_Handle, pADC->hdma);

  HAL_NVIC_SetPriority(dmaIRQ, 0, 0);  // Configure the NVIC for DMA
  HAL_NVIC_EnableIRQ(dmaIRQ);
}

/**************************************************************************************************\
* FUNCTION    Analog_init
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
void Analog_initADC(ADCSelect adcSel)
{
  ADCPort adcNum   = Analog_getPortNumber(adcSel);
  ADCPortCfg *pADC = &sAnalog.adc[adcNum];
  
  if (HAL_ADC_STATE_RESET != pADC->hadc.State)
    HAL_ADC_DeInit(&pADC->hadc);
  
  if (HAL_DMA_STATE_RESET != pADC->hdma.State)
    HAL_DMA_DeInit(&pADC->hdma);
  
  /*##-1- Configure the ADC peripheral #######################################*/
  switch (adcNum)
  {
    case ADC_PORT1: pADC->hadc.Instance = ADC1; break;
    case ADC_PORT2: pADC->hadc.Instance = ADC2; break;
    case ADC_PORT3: pADC->hadc.Instance = ADC3; break;
    default: return;
  } // 180MHz div6 = 30Mhz clock
  pADC->hadc.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV6;
  pADC->hadc.Init.Resolution            = ADC_RESOLUTION_12B;
  pADC->hadc.Init.ScanConvMode          = DISABLE;
  pADC->hadc.Init.ContinuousConvMode    = DISABLE;
  pADC->hadc.Init.DiscontinuousConvMode = DISABLE;
  pADC->hadc.Init.NbrOfDiscConversion   = 0;
  pADC->hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
  pADC->hadc.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T2_TRGO;
  pADC->hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  pADC->hadc.Init.NbrOfConversion       = 1;
  pADC->hadc.Init.DMAContinuousRequests = DISABLE;
  pADC->hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&pADC->hadc);
  
  /*##-2- Configure ADC regular channel ######################################*/
  ADC_ChannelConfTypeDef chanConfig;
  chanConfig.Channel = Analog_getChannelNumber(adcSel);
  chanConfig.Rank = 1;
  chanConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  chanConfig.Offset = 0;
  HAL_ADC_ConfigChannel(&pADC->hadc, &chanConfig);
  
  Analog_dmaInit(adcNum);
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCVal
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
uint32_t Analog_getChannelNumber(ADCSelect adcSelect)
{
  switch (adcSelect)
  {
    case ADC_VREF_INTERNAL:   return  ADC_CHANNEL_VREFINT;
    case ADC_DOM1_INCURRENT:  return  ADC_CHANNEL_1;
    case ADC_DOM2_INCURRENT:  return  ADC_CHANNEL_2;
    case ADC_DOM0_VOLTAGE:    return  ADC_CHANNEL_7;
    case ADC_DOM1_VOLTAGE:    return  ADC_CHANNEL_14;
    case ADC_DOM2_VOLTAGE:    return  ADC_CHANNEL_15;
    case ADC_DOM0_OUTCURRENT: return  ADC_CHANNEL_4;
    case ADC_DOM1_OUTCURRENT: return  ADC_CHANNEL_11;
    case ADC_DOM2_OUTCURRENT: return  ADC_CHANNEL_13;
    default: return 0;
  }
}

/**************************************************************************************************\
* FUNCTION    Analog_getPortNumber
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
ADCPort Analog_getPortNumber(ADCSelect adcSelect)
{
  switch (adcSelect)
  {
    case ADC_VREF_INTERNAL:   return ADC_PORT1;
    case ADC_DOM1_INCURRENT:  return ADC_PORT1;
    case ADC_DOM2_INCURRENT:  return ADC_PORT1;
    case ADC_DOM0_VOLTAGE:    return ADC_PORT2;
    case ADC_DOM1_VOLTAGE:    return ADC_PORT2;
    case ADC_DOM2_VOLTAGE:    return ADC_PORT2;
    case ADC_DOM0_OUTCURRENT: return ADC_PORT3;
    case ADC_DOM1_OUTCURRENT: return ADC_PORT3;
    case ADC_DOM2_OUTCURRENT: return ADC_PORT3;
    default: return ADC_PORT1;
  }
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCPortConfig
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
ADCPortCfg *Analog_getADCPortConfig(ADCSelect adcSelect)
{
  return &sAnalog.adc[(uint32_t)Analog_getPortNumber(adcSelect)];
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCHandle
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
ADC_HandleTypeDef *Analog_getADCHandle(ADCSelect adcSelect)
{
  ADCPortCfg *pCfg = Analog_getADCPortConfig(adcSelect);
  return &pCfg->hadc;
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCVal
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
uint16_t Analog_getADCVal(ADCSelect adc)
{
  ADCPort adcNum   = Analog_getPortNumber(adc);
  
  ADC_HandleTypeDef *hadc = Analog_getADCHandle(adc);
  
  /*##-1- Configure the ADC peripheral #######################################*/
  switch (adcNum)
  {
    case ADC_PORT1: hadc->Instance = ADC1; break;
    case ADC_PORT2: hadc->Instance = ADC2; break;
    case ADC_PORT3: hadc->Instance = ADC3; break;
    default: return 0;
  } // 180MHz div6 = 30Mhz clock
  hadc->Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV6;
  hadc->Init.Resolution            = ADC_RESOLUTION_12B;
  hadc->Init.ScanConvMode          = DISABLE;
  hadc->Init.ContinuousConvMode    = DISABLE;
  hadc->Init.DiscontinuousConvMode = DISABLE;
  hadc->Init.NbrOfDiscConversion   = 0;
  hadc->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc->Init.NbrOfConversion       = 1;
  hadc->Init.DMAContinuousRequests = DISABLE;
  hadc->Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(hadc);
  
  ADC_ChannelConfTypeDef sConfig;
  sConfig.Rank = 1;
  sConfig.Channel = Analog_getChannelNumber(adc);
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  HAL_ADC_ConfigChannel(hadc, &sConfig);
  
  HAL_ADC_Start(hadc);
  if (HAL_OK == HAL_ADC_PollForConversion(hadc, 10))
    return HAL_ADC_GetValue(hadc);
  else
    return 0;
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCVoltage
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
double Analog_getADCVoltage(ADCSelect adc, uint32_t numSamps)
{
  const double refVoltage = 1.21;
  uint32_t i = 0, refCode = 0, domCode = 0;
  for (i = 0; i < numSamps; i++)
  {
    refCode += Analog_getADCVal(ADC_VREF_INTERNAL);
    domCode += Analog_getADCVal(adc);
  }
  if ((ADC_DOM0_VOLTAGE == adc) || (ADC_DOM1_VOLTAGE == adc) || (ADC_DOM2_VOLTAGE == adc))
    domCode <<= 1;  // Domain voltages are divided by two before the ADC. Compensate here.
  return (refVoltage * ((double)domCode / (double)refCode));
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCCurrent
* DESCRIPTION 
* PARAMETERS  
* RETURNS     The value of the current channel in mA
\**************************************************************************************************/
double Analog_getADCCurrent(ADCSelect adc, const uint32_t numSamps)
{
  return ((Analog_getADCVoltage(adc, numSamps) / 0.1) / 100) * 1000;  // I = (V/R) / Gain
}

/**************************************************************************************************\
* FUNCTION    ADC_openPort
* DESCRIPTION Initializes the specified ADCPort with the provided AppLayer configuration
* PARAMETERS  port: The ADC port to open
*             adcConfig: The AppLayer configuration to apply to the port
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void Analog_openPort(ADCSelect adcSelect, AppADCConfig appConfig)
{
  ADCPortCfg *pCfg = Analog_getADCPortConfig(adcSelect);
  memcpy(&pCfg->cfg, &appConfig, sizeof(pCfg->cfg));
  
  ADC_ChannelConfTypeDef chanConfig;
  chanConfig.Rank = 1;
  chanConfig.Channel = Analog_getChannelNumber(adcSelect);
  chanConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  
  ADCPort adcPort = Analog_getPortNumber(adcSelect);
  Analog_initADC(adcSelect);
  
  HAL_ADC_ConfigChannel(&pCfg->hadc, &chanConfig);
  HAL_ADC_Start(&pCfg->hadc);
}

/**************************************************************************************************\
* FUNCTION    Analog_setupTimer
* DESCRIPTION Configures the ADCs to use DMA
* PARAMETERS  irqPeriod: The IRQ period in microseconds
* RETURNS     Nothing
\**************************************************************************************************/
boolean Analog_setupTimer(uint32_t irqPeriod)
{
  //uint32_t timFreq = HAL_RCC_GetPCLK1Freq() * 2;
  
  const uint32_t timFreq = 90000000;
  sAnalog.htim.Instance = TIM2;
  sAnalog.htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  sAnalog.htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
  //sAnalog.htim.Init.Period            = irqPeriod;
  //sAnalog.htim.Init.Prescaler         = (timFreq / (1000 * 1000)); // yields 1us per tick
  sAnalog.htim.Init.Period            = irqPeriod * (timFreq / (1000 * 1000));
  sAnalog.htim.Init.Prescaler         = 0;
  sAnalog.htim.Init.RepetitionCounter = 0;
  sAnalog.htim.Channel                = HAL_TIM_ACTIVE_CHANNEL_1;
  HAL_TIM_Base_Init(&sAnalog.htim);
  
  //TIM_ClockConfigTypeDef clockDef;
  //clockDef.ClockSource    = TIM_CLOCKSOURCE_INTERNAL;
  //clockDef.ClockPolarity  = TIM_CLOCKPOLARITY_BOTHEDGE;
  //clockDef.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  //clockDef.ClockFilter    = 0;
  //HAL_TIM_ConfigClockSource(&sAnalog.htim, &clockDef);
  
  TIM_MasterConfigTypeDef sMasterConfig;
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  //sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&sAnalog.htim, &sMasterConfig);
  
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    ADC_getSamples
* DESCRIPTION Configures an ADC port to provide a number of samples
* PARAMETERS  port: The ADC port to get samples from
*             numSamples: The number of samples after which the app will be notified
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void Analog_configureADC(ADCSelect adcSel, void *pDst, uint32_t numSamps)
{
  Analog_initADC(adcSel);
  ADCPortCfg *pCfg = Analog_getADCPortConfig(adcSel);
  pCfg->cfg.adcConfig.numSamps = numSamps;
  pCfg->cfg.appSampleBuffer = pDst;
  pCfg->cfg.adcConfig.chan[0].chanNum = Analog_getChannelNumber(adcSel);
  numSamps = MIN(0xFFFF, numSamps);
  if(HAL_ADC_Start_DMA(&pCfg->hadc, (uint32 *)pDst, numSamps) != HAL_OK)
    Error_Handler();
}

/**************************************************************************************************\
* FUNCTION    Analog_startSampleTimer
* DESCRIPTION Starts one of the sample timers that can (if configured) trigger and ADC conversion
* PARAMETERS  sampRate: The sample period (in microseconds)
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean Analog_startSampleTimer(uint32_t sampRate)
{
  Analog_setupTimer(sampRate);
  HAL_TIM_Base_Start_IT(&sAnalog.htim);
  
  NVIC_EnableIRQ(TIM2_IRQn);
  NVIC_ClearPendingIRQ(ADC_IRQn);
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Analog_stopSampleTimer
* DESCRIPTION Starts one of the sample timers that can (if configured) trigger and ADC conversion
* PARAMETERS  timer: The timer to start
*             reloadVal: The reload value for the specified timer
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean Analog_stopSampleTimer(void)
{
  HAL_TIM_Base_Stop_IT(&sAnalog.htim);
  HAL_ADC_Stop_DMA(&sAnalog.adc[ADC_PORT1].hadc);
  HAL_ADC_Stop_DMA(&sAnalog.adc[ADC_PORT2].hadc);
  HAL_ADC_Stop_DMA(&sAnalog.adc[ADC_PORT3].hadc);
  NVIC_DisableIRQ(TIM2_IRQn);
  return SUCCESS;
}

#include "stm32f429i_discovery.h"
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  
}

/**************************************************************************************************\
* FUNCTION      TIM2_IRQHandler
* DESCRIPTION   Handles interrupts originating from Timer2
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void TIM2_IRQHandler(void)
{
//  HAL_TIM_IRQHandler(&sAnalog.htim);
  LED3_GPIO_PORT->ODR ^= LED3_PIN;
  //TIM2->EGR = TIM_EGR_UG;
  TIM2->SR = ~(TIM_IT_UPDATE);
  Tests_notifySampleTrigger();
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
  HAL_DMA_IRQHandler(sAnalog.adc[ADC_PORT1].hadc.DMA_Handle);
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
  HAL_DMA_IRQHandler(sAnalog.adc[ADC_PORT3].hadc.DMA_Handle);
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
  HAL_DMA_IRQHandler(sAnalog.adc[ADC_PORT2].hadc.DMA_Handle);
}
