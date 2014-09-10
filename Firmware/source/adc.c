#include "stm32f2xx.h"
#include "adc.h"
#include "dma.h"
#include "eeprom.h"
#include "gpio.h"
#include "util.h"

#define ADC_Mode_Independent                       ((uint32_t)0x00000000)
#define ADC_DualMode_RegSimult_InjecSimult         ((uint32_t)0x00000001)
#define ADC_DualMode_RegSimult_AlterTrig           ((uint32_t)0x00000002)
#define ADC_DualMode_InjecSimult                   ((uint32_t)0x00000005)
#define ADC_DualMode_RegSimult                     ((uint32_t)0x00000006)
#define ADC_DualMode_Interl                        ((uint32_t)0x00000007)
#define ADC_DualMode_AlterTrig                     ((uint32_t)0x00000009)
#define ADC_TripleMode_RegSimult_InjecSimult       ((uint32_t)0x00000011)
#define ADC_TripleMode_RegSimult_AlterTrig         ((uint32_t)0x00000012)
#define ADC_TripleMode_InjecSimult                 ((uint32_t)0x00000015)
#define ADC_TripleMode_RegSimult                   ((uint32_t)0x00000016)
#define ADC_TripleMode_Interl                      ((uint32_t)0x00000017)
#define ADC_TripleMode_AlterTrig                   ((uint32_t)0x00000019)

#define ADC_Prescaler_Div2                         ((uint32_t)0x00000000)
#define ADC_Prescaler_Div4                         ((uint32_t)0x00010000)
#define ADC_Prescaler_Div6                         ((uint32_t)0x00020000)
#define ADC_Prescaler_Div8                         ((uint32_t)0x00030000)

/* DMA mode disabled */
#define ADC_DMAAccessMode_Disabled      ((uint32_t)0x00000000)
/* DMA mode 1 enabled (2 / 3 half-words one by one - 1 then 2 then 3)*/
#define ADC_DMAAccessMode_1             ((uint32_t)0x00004000)
/* DMA mode 2 enabled (2 / 3 half-words by pairs - 2&1 then 1&3 then 3&2)*/
#define ADC_DMAAccessMode_2             ((uint32_t)0x00008000)
/* DMA mode 3 enabled (2 / 3 bytes by pairs - 2&1 then 1&3 then 3&2) */
#define ADC_DMAAccessMode_3             ((uint32_t)0x0000C000)

#define ADC_TwoSamplingDelay_5Cycles               ((uint32_t)0x00000000)
#define ADC_TwoSamplingDelay_6Cycles               ((uint32_t)0x00000100)
#define ADC_TwoSamplingDelay_7Cycles               ((uint32_t)0x00000200)
#define ADC_TwoSamplingDelay_8Cycles               ((uint32_t)0x00000300)
#define ADC_TwoSamplingDelay_9Cycles               ((uint32_t)0x00000400)
#define ADC_TwoSamplingDelay_10Cycles              ((uint32_t)0x00000500)
#define ADC_TwoSamplingDelay_11Cycles              ((uint32_t)0x00000600)
#define ADC_TwoSamplingDelay_12Cycles              ((uint32_t)0x00000700)
#define ADC_TwoSamplingDelay_13Cycles              ((uint32_t)0x00000800)
#define ADC_TwoSamplingDelay_14Cycles              ((uint32_t)0x00000900)
#define ADC_TwoSamplingDelay_15Cycles              ((uint32_t)0x00000A00)
#define ADC_TwoSamplingDelay_16Cycles              ((uint32_t)0x00000B00)
#define ADC_TwoSamplingDelay_17Cycles              ((uint32_t)0x00000C00)
#define ADC_TwoSamplingDelay_18Cycles              ((uint32_t)0x00000D00)
#define ADC_TwoSamplingDelay_19Cycles              ((uint32_t)0x00000E00)
#define ADC_TwoSamplingDelay_20Cycles              ((uint32_t)0x00000F00)

#define ADC_Resolution_12b                         ((uint32_t)0x00000000)
#define ADC_Resolution_10b                         ((uint32_t)0x01000000)
#define ADC_Resolution_8b                          ((uint32_t)0x02000000)
#define ADC_Resolution_6b                          ((uint32_t)0x03000000)

#define ADC_ExternalTrigConvEdge_None              ((uint32_t)0x00000000)
#define ADC_ExternalTrigConvEdge_Rising            ((uint32_t)0x10000000)
#define ADC_ExternalTrigConvEdge_Falling           ((uint32_t)0x20000000)
#define ADC_ExternalTrigConvEdge_RisingFalling     ((uint32_t)0x30000000)

#define ADC_ExternalTrigConv_T1_CC1                ((uint32_t)0x00000000)
#define ADC_ExternalTrigConv_T1_CC2                ((uint32_t)0x01000000)
#define ADC_ExternalTrigConv_T1_CC3                ((uint32_t)0x02000000)
#define ADC_ExternalTrigConv_T2_CC2                ((uint32_t)0x03000000)
#define ADC_ExternalTrigConv_T2_CC3                ((uint32_t)0x04000000)
#define ADC_ExternalTrigConv_T2_CC4                ((uint32_t)0x05000000)
#define ADC_ExternalTrigConv_T2_TRGO               ((uint32_t)0x06000000)
#define ADC_ExternalTrigConv_T3_CC1                ((uint32_t)0x07000000)
#define ADC_ExternalTrigConv_T3_TRGO               ((uint32_t)0x08000000)
#define ADC_ExternalTrigConv_T4_CC4                ((uint32_t)0x09000000)
#define ADC_ExternalTrigConv_T5_CC1                ((uint32_t)0x0A000000)
#define ADC_ExternalTrigConv_T5_CC2                ((uint32_t)0x0B000000)
#define ADC_ExternalTrigConv_T5_CC3                ((uint32_t)0x0C000000)
#define ADC_ExternalTrigConv_T8_CC1                ((uint32_t)0x0D000000)
#define ADC_ExternalTrigConv_T8_TRGO               ((uint32_t)0x0E000000)
#define ADC_ExternalTrigConv_Ext_IT11              ((uint32_t)0x0F000000)

#define ADC_DataAlign_Right                        ((uint32_t)0x00000000)
#define ADC_DataAlign_Left                         ((uint32_t)0x00000800)

#define ADC_AnalogWatchdog_SingleRegEnable         ((uint32_t)0x00800200)
#define ADC_AnalogWatchdog_SingleInjecEnable       ((uint32_t)0x00400200)
#define ADC_AnalogWatchdog_SingleRegOrInjecEnable  ((uint32_t)0x00C00200)
#define ADC_AnalogWatchdog_AllRegEnable            ((uint32_t)0x00800000)
#define ADC_AnalogWatchdog_AllInjecEnable          ((uint32_t)0x00400000)
#define ADC_AnalogWatchdog_AllRegAllInjecEnable    ((uint32_t)0x00C00000)
#define ADC_AnalogWatchdog_None                    ((uint32_t)0x00000000)

#define ADC_IT_EOC                                 ((uint16_t)0x0205)
#define ADC_IT_AWD                                 ((uint16_t)0x0106)
#define ADC_IT_JEOC                                ((uint16_t)0x0407)
#define ADC_IT_OVR                                 ((uint16_t)0x201A)

#define ADC_FLAG_AWD                               ((uint8_t)0x01)
#define ADC_FLAG_EOC                               ((uint8_t)0x02)
#define ADC_FLAG_JEOC                              ((uint8_t)0x04)
#define ADC_FLAG_JSTRT                             ((uint8_t)0x08)
#define ADC_FLAG_STRT                              ((uint8_t)0x10)
#define ADC_FLAG_OVR                               ((uint8_t)0x20)

#define CR1_DISCNUM_RESET         ((uint32_t)0xFFFF1FFF)
#define CR1_AWDCH_RESET           ((uint32_t)0xFFFFFFE0)
#define CR1_AWDMode_RESET         ((uint32_t)0xFF3FFDFF)
#define CR1_CLEAR_MASK            ((uint32_t)0xFCFFFEFF)
#define CR2_EXTEN_RESET           ((uint32_t)0xCFFFFFFF)
#define CR2_JEXTEN_RESET          ((uint32_t)0xFFCFFFFF)
#define CR2_JEXTSEL_RESET         ((uint32_t)0xFFF0FFFF)
#define CR2_CLEAR_MASK            ((uint32_t)0xC0FFF7FD)
#define SQR3_SQ_SET               ((uint32_t)0x0000001F)
#define SQR2_SQ_SET               ((uint32_t)0x0000001F)
#define SQR1_SQ_SET               ((uint32_t)0x0000001F)
#define SQR1_L_RESET              ((uint32_t)0xFF0FFFFF)
#define JSQR_JSQ_SET              ((uint32_t)0x0000001F)
#define JSQR_JL_SET               ((uint32_t)0x00300000)
#define JSQR_JL_RESET             ((uint32_t)0xFFCFFFFF)
#define SMPR1_SMP_SET             ((uint32_t)0x00000007)
#define SMPR2_SMP_SET             ((uint32_t)0x00000007)
#define JDR_OFFSET                ((uint8_t)0x28)
#define CDR_ADDRESS               ((uint32_t)0x40012308)
#define CR_CLEAR_MASK             ((uint32_t)0xFFFC30E0)

typedef struct
{
  uint32_t        ADC_Resolution;
  FunctionalState ADC_ScanConvMode;
  FunctionalState ADC_ContinuousConvMode;
  uint32_t        ADC_ExternalTrigConvEdge;
  uint32_t        ADC_ExternalTrigConv;
  uint32_t        ADC_DataAlign;
  uint8_t         ADC_NbrOfConversion;
} ADC_InitTypeDef;

typedef struct
{
  uint32_t ADC_Mode;
  uint32_t ADC_Prescaler;
  uint32_t ADC_DMAAccessMode;
  uint32_t ADC_TwoSamplingDelay;
} ADC_CommonInitTypeDef;

// CommStatus is maintained internally but toSend and toReceive are populated via appLayer requests
typedef struct
{
  uint8  currChan;
  uint16 sampBufIdx;
  uint16 samplesRemaining;
} ADCStatus;

// CommPort contains the information required for an appLayer agent to send and receive data
typedef struct
{
  boolean       isConfigured; // Indicates if this port is registered to an appLayer
  ADC_TypeDef  *pADC;         // The base register for hardware ADC interaction
  AppADCConfig  appConfig;    // Buffers and callbacks set by the appLayer
  ADCStatus     adcStatus;   // State information for sending and receiving
} ADConverter;

struct
{
  ADConverter adc[NUM_ADC_PORTS];
} sADC;

/* Initialization and Configuration functions *********************************/
void ADC_configureADC(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct);
void ADC_StructInit(ADC_InitTypeDef* ADC_InitStruct);
void ADC_CommonInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct);
void ADC_CommonStructInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct);
void ADC_Cmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* Temperature Sensor, Vrefint and VBAT management functions ******************/
void ADC_TempSensorVrefintCmd(FunctionalState NewState);

/* Regular Channels Configuration functions ***********************************/
void ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime);
uint16_t ADC_GetConversionValue(ADC_TypeDef* ADCx);
uint32_t ADC_GetMultiModeConversionValue(void);

/* Interrupts and flags management functions **********************************/
void ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState);
FlagStatus ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG);
void ADC_ClearFlag(ADC_TypeDef* ADCx, uint8_t ADC_FLAG);
ITStatus ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT);
void ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint16_t ADC_IT);

/**************************************************************************************************\
* FUNCTION    ADC_init
* DESCRIPTION Initializes the analog to digital converters to their default state
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       ADCClk is @30MHz without modifying any prescalars (APB2/2)
\**************************************************************************************************/
void ADC_init(void)
{
  const uint16 ctrlPortA = (GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_6);

  // Initialize all potential ADC pins to analog configuration
  GPIO_InitTypeDef analogCtrlPortA = {ctrlPortA, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOA, TRUE);
  GPIO_configurePins(GPIOA, &analogCtrlPortA);

  // Enable ADC clocks so that peripheral configuration changes will stick
  RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN | RCC_APB2ENR_ADC3EN);

  // Enable the internal temperature sensor and vRef channels
  ADC_TempSensorVrefintCmd(ENABLE);

  Util_fillMemory(&sADC, sizeof(sADC), 0x00); // Zero out the sADC structure
}

static ADC_TypeDef *ADC_getPortPtr(ADCPort port)
{
  switch (port)
  {
    case ADC_PORT1:
      return ADC1;
    case ADC_PORT2:
      return ADC2;
    case ADC_PORT3:
      return ADC3;
    default:
      return NULL;
  }
}

void ADC_DMACmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  if (NewState != DISABLE)
  {
    /* Enable the selected ADC DMA request */
    ADCx->CR2 |= (uint32_t)ADC_CR2_DMA;
  }
  else
  {
    /* Disable the selected ADC DMA request */
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_DMA);
  }
}

void ADC_DMARequestAfterLastTransferCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
  if (NewState != DISABLE)
  {
    /* Enable the selected ADC DMA request after last transfer */
    ADCx->CR2 |= (uint32_t)ADC_CR2_DDS;
  }
  else
  {
    /* Disable the selected ADC DMA request after last transfer */
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_DDS);
  }
}

void ADC_MultiModeDMARequestAfterLastTransferCmd(FunctionalState NewState)
{
  if (NewState != DISABLE)
  {
    /* Enable the selected ADC DMA request after last transfer */
    ADC->CCR |= (uint32_t)ADC_CCR_DDS;
  }
  else
  {
    /* Disable the selected ADC DMA request after last transfer */
    ADC->CCR &= (uint32_t)(~ADC_CCR_DDS);
  }
}

void ADC_configureADC(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct)
{
  uint32_t tmpreg1 = 0;
  uint8_t tmpreg2 = 0;

  /*---------------------------- ADCx CR1 Configuration -----------------*/
  /* Get the ADCx CR1 value */
  tmpreg1 = ADCx->CR1;

  /* Clear RES and SCAN bits */
  tmpreg1 &= CR1_CLEAR_MASK;

  /* Configure ADCx: scan conversion mode and resolution */
  /* Set SCAN bit according to ADC_ScanConvMode value */
  /* Set RES bit according to ADC_Resolution value */
  tmpreg1 |= (uint32_t)(((uint32_t)ADC_InitStruct->ADC_ScanConvMode << 8) | \
                                   ADC_InitStruct->ADC_Resolution);
  /* Write to ADCx CR1 */
  ADCx->CR1 = tmpreg1;
  /*---------------------------- ADCx CR2 Configuration -----------------*/
  /* Get the ADCx CR2 value */
  tmpreg1 = ADCx->CR2;

  /* Clear CONT, ALIGN, EXTEN and EXTSEL bits */
  tmpreg1 &= CR2_CLEAR_MASK;

  /* Configure ADCx: external trigger event and edge, data alignment and
     continuous conversion mode */
  /* Set ALIGN bit according to ADC_DataAlign value */
  /* Set EXTEN bits according to ADC_ExternalTrigConvEdge value */
  /* Set EXTSEL bits according to ADC_ExternalTrigConv value */
  /* Set CONT bit according to ADC_ContinuousConvMode value */
  tmpreg1 |= (uint32_t)(ADC_InitStruct->ADC_DataAlign | \
                        ADC_InitStruct->ADC_ExternalTrigConv |
                        ADC_InitStruct->ADC_ExternalTrigConvEdge | \
                        ((uint32_t)ADC_InitStruct->ADC_ContinuousConvMode << 1));

  /* Write to ADCx CR2 */
  ADCx->CR2 = tmpreg1;
  /*---------------------------- ADCx SQR1 Configuration -----------------*/
  /* Get the ADCx SQR1 value */
  tmpreg1 = ADCx->SQR1;

  /* Clear L bits */
  tmpreg1 &= SQR1_L_RESET;

  /* Configure ADCx: regular channel sequence length */
  /* Set L bits according to ADC_NbrOfConversion value */
  tmpreg2 |= (uint8_t)(ADC_InitStruct->ADC_NbrOfConversion - (uint8_t)1);
  tmpreg1 |= ((uint32_t)tmpreg2 << 20);

  /* Write to ADCx SQR1 */
  ADCx->SQR1 = tmpreg1;
}

void ADC_StructInit(ADC_InitTypeDef* ADC_InitStruct)
{
  /* Initialize the ADC_Mode member */
  ADC_InitStruct->ADC_Resolution = ADC_Resolution_12b;

  /* initialize the ADC_ScanConvMode member */
  ADC_InitStruct->ADC_ScanConvMode = DISABLE;

  /* Initialize the ADC_ContinuousConvMode member */
  ADC_InitStruct->ADC_ContinuousConvMode = DISABLE;

  /* Initialize the ADC_ExternalTrigConvEdge member */
  ADC_InitStruct->ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;

  /* Initialize the ADC_ExternalTrigConv member */
  ADC_InitStruct->ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;

  /* Initialize the ADC_DataAlign member */
  ADC_InitStruct->ADC_DataAlign = ADC_DataAlign_Right;

  /* Initialize the ADC_NbrOfConversion member */
  ADC_InitStruct->ADC_NbrOfConversion = 1;
}

void ADC_CommonInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)
{
  uint32_t tmpreg1 = 0;
  /*---------------------------- ADC CCR Configuration -----------------*/
  /* Get the ADC CCR value */
  tmpreg1 = ADC->CCR;

  /* Clear MULTI, DELAY, DMA and ADCPRE bits */
  tmpreg1 &= CR_CLEAR_MASK;

  /* Configure ADCx: Multi mode, Delay between two sampling time, ADC prescaler,
     and DMA access mode for multimode */
  /* Set MULTI bits according to ADC_Mode value */
  /* Set ADCPRE bits according to ADC_Prescaler value */
  /* Set DMA bits according to ADC_DMAAccessMode value */
  /* Set DELAY bits according to ADC_TwoSamplingDelay value */
  tmpreg1 |= (uint32_t)(ADC_CommonInitStruct->ADC_Mode |
                        ADC_CommonInitStruct->ADC_Prescaler |
                        ADC_CommonInitStruct->ADC_DMAAccessMode |
                        ADC_CommonInitStruct->ADC_TwoSamplingDelay);

  /* Write to ADC CCR */
  ADC->CCR = tmpreg1;
}

/**
  * @brief  Fills each ADC_CommonInitStruct member with its default value.
  * @param  ADC_CommonInitStruct: pointer to an ADC_CommonInitTypeDef structure
  *         which will be initialized.
  * @retval None
  */
void ADC_CommonStructInit(ADC_CommonInitTypeDef* ADC_CommonInitStruct)
{
  /* Initialize the ADC_Mode member */
  ADC_CommonInitStruct->ADC_Mode = ADC_Mode_Independent;

  /* initialize the ADC_Prescaler member */
  ADC_CommonInitStruct->ADC_Prescaler = ADC_Prescaler_Div2;

  /* Initialize the ADC_DMAAccessMode member */
  ADC_CommonInitStruct->ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;

  /* Initialize the ADC_TwoSamplingDelay member */
  ADC_CommonInitStruct->ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
}

/**************************************************************************************************\
* FUNCTION    ADC_Cmd
* DESCRIPTION Turns the selected ADC on or off
* PARAMETERS  ADCx: The ADC port to enable/disable
*             newState: The state to place the ADC into
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void ADC_Cmd(ADC_TypeDef* ADCx, FunctionalState newState)
{
  if (newState != DISABLE)
    ADCx->CR2 |= (uint32_t)ADC_CR2_ADON;    // Set the ADON bit to wake up the ADC
  else
    ADCx->CR2 &= (uint32_t)(~ADC_CR2_ADON); // Disable the selected ADC peripheral
}

/**************************************************************************************************\
* FUNCTION    ADC_openPort
* DESCRIPTION Initializes the specified ADCPort with the provided AppLayer configuration
* PARAMETERS  port: The ADC port to open
*             adcConfig: The AppLayer configuration to apply to the port
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void ADC_openPort(ADCPort port, AppADCConfig appConfig)
{
  ADC_InitTypeDef adcInit;
  ADC_CommonInitTypeDef adcCommonInit;
  ADCConfig *pCfg = &appConfig.adcConfig;

  // Copy the configuration information into sADC
  Util_fillMemory((uint8*)&sADC.adc[port], sizeof(sADC.adc[port]), 0x00);
  Util_copyMemory((uint8*)&appConfig, (uint8*)&sADC.adc[port].appConfig, sizeof(appConfig));
  sADC.adc[port].pADC = ADC_getPortPtr(port);
  sADC.adc[port].isConfigured = TRUE;

  switch (port)
  {
    case ADC_PORT1:
      DMA_DeInit(DMA2_Stream0);
      break;
    case ADC_PORT2:
      DMA_DeInit(DMA2_Stream2);
      break;
    case ADC_PORT3:
      DMA_DeInit(DMA2_Stream1);
      break;
    default:
      return;
  }

  // Initialize the structure common to all ADCs
  ADC_CommonStructInit(&adcCommonInit);
  ADC_CommonInit(&adcCommonInit);

  // Initialize the individual ADC for DMA transfer mode
  ADC_StructInit(&adcInit);
  adcInit.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
  adcInit.ADC_ExternalTrigConv     = ADC_ExternalTrigConv_T3_TRGO;
  ADC_configureADC(sADC.adc[port].pADC, &adcInit);
  ADC_RegularChannelConfig(sADC.adc[port].pADC, pCfg->chan[0].chanNum, 1, pCfg->chan[0].sampleTime);
  sADC.adc[port].adcStatus.currChan = pCfg->chan[0].chanNum;

  ADC_DMACmd(sADC.adc[port].pADC, ENABLE);

  ADC_DMARequestAfterLastTransferCmd(sADC.adc[port].pADC, ENABLE);

  ADC_ClearFlag(sADC.adc[port].pADC, ADC_SR_OVR); // Clear any previous overrun errors

//  ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);

//  ADC_ITConfig(sADC.adc[port].pADC, ADC_IT_EOC, ENABLE); // Enable interrupts

//  NVIC_EnableIRQ(ADC_IRQn);
}

/**************************************************************************************************\
* FUNCTION    ADC_getSamples
* DESCRIPTION Configures an ADC port to provide a number of samples
* PARAMETERS  port: The ADC port to get samples from
*             numSamples: The number of samples after which the app will be notified
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void ADC_getSamples(ADCPort port, uint16 numSamples)
{
  DMA_InitTypeDef dmaInit;
  sADC.adc[port].adcStatus.samplesRemaining = numSamples;
  sADC.adc[port].adcStatus.sampBufIdx = 0;

  DMA_StructInit(&dmaInit);
  dmaInit.DMA_Memory0BaseAddr = (uint32)&sADC.adc[port].appConfig.appSampleBuffer[0];
  dmaInit.DMA_PeripheralBaseAddr = ((uint32)sADC.adc[port].pADC) + ADC_DR_OFFSET;
  dmaInit.DMA_BufferSize = sADC.adc[port].adcStatus.samplesRemaining;
  dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dmaInit.DMA_Priority = DMA_Priority_Medium;
  dmaInit.DMA_FIFOMode = DMA_FIFOMode_Enable;
  dmaInit.DMA_FIFOThreshold = DMA_FIFOStatus_HalfFull;

  switch (port) // Enable DMA Stream Half / Transfer Complete interrupt
  {
    case ADC_PORT1:
      dmaInit.DMA_Channel = DMA_Channel_0;
      DMA_Init(DMA2_Stream0, &dmaInit);
      DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
      DMA_Cmd(DMA2_Stream0, ENABLE, sADC.adc[port].appConfig.appNotifyConversionComplete);
      NVIC_EnableIRQ(DMA2_Stream0_IRQn);
      break;
    case ADC_PORT2:
      dmaInit.DMA_Channel = DMA_Channel_1;
      DMA_Init(DMA2_Stream2, &dmaInit);
      DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, ENABLE);
      DMA_Cmd(DMA2_Stream2, ENABLE, sADC.adc[port].appConfig.appNotifyConversionComplete);
      NVIC_EnableIRQ(DMA2_Stream2_IRQn);
      break;
    case ADC_PORT3:
      dmaInit.DMA_Channel = DMA_Channel_2;
      DMA_Init(DMA2_Stream1, &dmaInit);
      DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);
      DMA_Cmd(DMA2_Stream1, ENABLE, sADC.adc[port].appConfig.appNotifyConversionComplete);
      NVIC_EnableIRQ(DMA2_Stream1_IRQn);
      break;
    default:
      return;
  }
  ADC_Cmd(sADC.adc[port].pADC, ENABLE); // Turn on the ADC
}

/**************************************************************************************************\
* FUNCTION    ADC_startSampleTimer
* DESCRIPTION Starts one of the sample timers that can (if configured) trigger and ADC conversion
* PARAMETERS  timer: The timer to start
*             reloadVal: The reload value for the specified timer
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean ADC_startSampleTimer(HardTimer timer, uint16 reloadVal)
{
  switch (timer)
  {
    case TIME_HARD_TIMER_TIMER3:
      Time_initTimer3(reloadVal);
      break;
    case TIME_HARD_TIMER_TIMER2:
      Time_initTimer2(reloadVal);
      break;
    case TIME_HARD_TIMER_TIMER8:  // Timer8 not configured for ADCs yet
    default:      // Cannot use any other timer for gating ADCs
      return ERROR;
  }
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    ADC_stopSampleTimer
* DESCRIPTION Starts one of the sample timers that can (if configured) trigger and ADC conversion
* PARAMETERS  timer: The timer to start
*             reloadVal: The reload value for the specified timer
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean ADC_stopSampleTimer(HardTimer timer)
{
  Time_stopTimer(timer);
  return SUCCESS;
}

void ADC_TempSensorVrefintCmd(FunctionalState NewState)
{
  if (NewState != DISABLE)
    ADC->CCR |= (uint32_t)ADC_CCR_TSVREFE;    // Enable the temperature sensor and Vrefint channel
  else
    ADC->CCR &= (uint32_t)(~ADC_CCR_TSVREFE); // Disable the temperature sensor and Vrefint channel
}

void ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime)
{
  uint32_t tmpreg1 = 0, tmpreg2 = 0;

  /* if ADC_Channel_10 ... ADC_Channel_18 is selected */
  if (ADC_Channel > ADC_Channel_9)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR1;

    /* Calculate the mask to clear */
    tmpreg2 = SMPR1_SMP_SET << (3 * (ADC_Channel - 10));

    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;

    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * (ADC_Channel - 10));

    /* Set the new sample time */
    tmpreg1 |= tmpreg2;

    /* Store the new register value */
    ADCx->SMPR1 = tmpreg1;
  }
  else /* ADC_Channel include in ADC_Channel_[0..9] */
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SMPR2;

    /* Calculate the mask to clear */
    tmpreg2 = SMPR2_SMP_SET << (3 * ADC_Channel);

    /* Clear the old sample time */
    tmpreg1 &= ~tmpreg2;

    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_SampleTime << (3 * ADC_Channel);

    /* Set the new sample time */
    tmpreg1 |= tmpreg2;

    /* Store the new register value */
    ADCx->SMPR2 = tmpreg1;
  }
  /* For Rank 1 to 6 */
  if (Rank < 7)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR3;

    /* Calculate the mask to clear */
    tmpreg2 = SQR3_SQ_SET << (5 * (Rank - 1));

    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;

    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 1));

    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;

    /* Store the new register value */
    ADCx->SQR3 = tmpreg1;
  }
  /* For Rank 7 to 12 */
  else if (Rank < 13)
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR2;

    /* Calculate the mask to clear */
    tmpreg2 = SQR2_SQ_SET << (5 * (Rank - 7));

    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;

    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 7));

    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;

    /* Store the new register value */
    ADCx->SQR2 = tmpreg1;
  }
  /* For Rank 13 to 16 */
  else
  {
    /* Get the old register value */
    tmpreg1 = ADCx->SQR1;

    /* Calculate the mask to clear */
    tmpreg2 = SQR1_SQ_SET << (5 * (Rank - 13));

    /* Clear the old SQx bits for the selected rank */
    tmpreg1 &= ~tmpreg2;

    /* Calculate the mask to set */
    tmpreg2 = (uint32_t)ADC_Channel << (5 * (Rank - 13));

    /* Set the SQx bits for the selected rank */
    tmpreg1 |= tmpreg2;

    /* Store the new register value */
    ADCx->SQR1 = tmpreg1;
  }
}

uint16 ADC_GetConversionValue(ADC_TypeDef* ADCx)
{
  return (uint16_t) ADCx->DR; // Return the selected ADC conversion value
}

/**
  * @brief  Returns the last ADC1, ADC2 and ADC3 regular conversions results
  *         data in the selected multi mode.
  * @param  None
  * @retval The Data conversion value.
  * @note   In dual mode, the value returned by this function is as following
  *           Data[15:0] : these bits contain the regular data of ADC1.
  *           Data[31:16]: these bits contain the regular data of ADC2.
  * @note   In triple mode, the value returned by this function is as following
  *           Data[15:0] : these bits contain alternatively the regular data of ADC1, ADC3 and ADC2.
  *           Data[31:16]: these bits contain alternatively the regular data of ADC2, ADC1 and ADC3.
  */
uint32_t ADC_GetMultiModeConversionValue(void)
{
  return (*(volatile uint32_t *)CDR_ADDRESS); // Return the multi mode conversion value
}

static void ADC_handleEndOfConversionInterrupt(ADCPort port)
{
  AppADCConfig *pConfig = &sADC.adc[port].appConfig;
  ADCStatus    *pStatus = &sADC.adc[port].adcStatus;

  if (pStatus->samplesRemaining-- > 0)
    pConfig->appSampleBuffer[pStatus->sampBufIdx++] = ADC_GetConversionValue(sADC.adc[port].pADC);
  else
  {
    pConfig->appNotifyConversionComplete(pStatus->currChan, pStatus->sampBufIdx);
    ADC_Cmd(sADC.adc[port].pADC, DISABLE); // Disable further interrupts from this ADC
  }
}

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is done
 *----------------------------------------------------------------------------*/
void ADC_IRQHandler(void)
{
  ITStatus adc1IntStatus = ADC_GetITStatus(ADC1, ADC_IT_EOC);
  ITStatus adc2IntStatus = ADC_GetITStatus(ADC2, ADC_IT_EOC);
  ITStatus adc3IntStatus = ADC_GetITStatus(ADC3, ADC_IT_EOC);

  if (adc1IntStatus == SET) // ADC1 EOC interrupt?
    ADC_handleEndOfConversionInterrupt(ADC_PORT1);
  
  if (adc2IntStatus == SET) // ADC1 EOC interrupt?
    ADC_handleEndOfConversionInterrupt(ADC_PORT2);
  
  if (adc3IntStatus == SET) // ADC1 EOC interrupt?
    ADC_handleEndOfConversionInterrupt(ADC_PORT3);
}

void ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState)
{
  uint32_t itmask = 0;

  /* Get the ADC IT index */
  itmask = (uint8_t)ADC_IT;
  itmask = (uint32_t)0x01 << itmask;

  if (NewState != DISABLE)
  {
    /* Enable the selected ADC interrupts */
    ADCx->CR1 |= itmask;
  }
  else
  {
    /* Disable the selected ADC interrupts */
    ADCx->CR1 &= (~(uint32_t)itmask);
  }
}

/**
  * @brief  Checks whether the specified ADC flag is set or not.
  * @param  ADCx: where x can be 1, 2 or 3 to select the ADC peripheral.
  * @param  ADC_FLAG: specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg ADC_FLAG_AWD: Analog watchdog flag
  *            @arg ADC_FLAG_EOC: End of conversion flag
  *            @arg ADC_FLAG_JEOC: End of injected group conversion flag
  *            @arg ADC_FLAG_JSTRT: Start of injected group conversion flag
  *            @arg ADC_FLAG_STRT: Start of regular group conversion flag
  *            @arg ADC_FLAG_OVR: Overrun flag
  * @retval The new state of ADC_FLAG (SET or RESET).
  */
FlagStatus ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{
  FlagStatus bitstatus = RESET;

  /* Check the status of the specified ADC flag */
  if ((ADCx->SR & ADC_FLAG) != (uint8_t)RESET)
  {
    /* ADC_FLAG is set */
    bitstatus = SET;
  }
  else
  {
    /* ADC_FLAG is reset */
    bitstatus = RESET;
  }
  /* Return the ADC_FLAG status */
  return  bitstatus;
}

/**
  * @brief  Clears the ADCx's pending flags.
  * @param  ADCx: where x can be 1, 2 or 3 to select the ADC peripheral.
  * @param  ADC_FLAG: specifies the flag to clear.
  *          This parameter can be any combination of the following values:
  *            @arg ADC_FLAG_AWD: Analog watchdog flag
  *            @arg ADC_FLAG_EOC: End of conversion flag
  *            @arg ADC_FLAG_JEOC: End of injected group conversion flag
  *            @arg ADC_FLAG_JSTRT: Start of injected group conversion flag
  *            @arg ADC_FLAG_STRT: Start of regular group conversion flag
  *            @arg ADC_FLAG_OVR: Overrun flag
  * @retval None
  */
void ADC_ClearFlag(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{
  /* Clear the selected ADC flags */
  ADCx->SR = ~(uint32_t)ADC_FLAG;
}

/**
  * @brief  Checks whether the specified ADC interrupt has occurred or not.
  * @param  ADCx:   where x can be 1, 2 or 3 to select the ADC peripheral.
  * @param  ADC_IT: specifies the ADC interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg ADC_IT_EOC: End of conversion interrupt mask
  *            @arg ADC_IT_AWD: Analog watchdog interrupt mask
  *            @arg ADC_IT_JEOC: End of injected conversion interrupt mask
  *            @arg ADC_IT_OVR: Overrun interrupt mask
  * @retval The new state of ADC_IT (SET or RESET).
  */
ITStatus ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t itmask = 0, enablestatus = 0;

  /* Get the ADC IT index */
  itmask = ADC_IT >> 8;

  /* Get the ADC_IT enable bit status */
  enablestatus = (ADCx->CR1 & ((uint32_t)0x01 << (uint8_t)ADC_IT)) ;

  /* Check the status of the specified ADC interrupt */
  if (((ADCx->SR & itmask) != (uint32_t)RESET) && enablestatus)
  {
    /* ADC_IT is set */
    bitstatus = SET;
  }
  else
  {
    /* ADC_IT is reset */
    bitstatus = RESET;
  }
  /* Return the ADC_IT status */
  return  bitstatus;
}

/**
  * @brief  Clears the ADCx's interrupt pending bits.
  * @param  ADCx: where x can be 1, 2 or 3 to select the ADC peripheral.
  * @param  ADC_IT: specifies the ADC interrupt pending bit to clear.
  *          This parameter can be one of the following values:
  *            @arg ADC_IT_EOC: End of conversion interrupt mask
  *            @arg ADC_IT_AWD: Analog watchdog interrupt mask
  *            @arg ADC_IT_JEOC: End of injected conversion interrupt mask
  *            @arg ADC_IT_OVR: Overrun interrupt mask
  * @retval None
  */
void ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
  uint8_t itmask = 0;
  /* Get the ADC IT index */
  itmask = (uint8_t)(ADC_IT >> 8);
  /* Clear the selected ADC interrupt pending bits */
  ADCx->SR = ~(uint32_t)itmask;
}

float ADC_convertInternalTemperature(uint16 adcVal)
{
  float temp = (float)adcVal;
  temp *= 3.3 / 4095.0;
  temp  = temp -.76;
  temp *= .0025;
  return temp;
}

#define FILE_ID ADC_C
