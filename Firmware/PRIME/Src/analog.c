#include "analog.h"
#include <string.h>

#define FILE_ID ANALOG_C

static struct
{
  AnalogInit cfg;
  ADC_HandleTypeDef hadc1;
  ADC_HandleTypeDef hadc2;
  ADC_HandleTypeDef hadc3;
  DMA_HandleTypeDef hdma_adc1;
  DMA_HandleTypeDef hdma_adc2;
  DMA_HandleTypeDef hdma_adc3;
} sAnalog;

/**************************************************************************************************\
* FUNCTION    Analog_init
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool Analog_init(AnalogInit *pAnInit)
{
  memset(&sAnalog, 0x00, sizeof(sAnalog));
  memcpy(&sAnalog.hadc1, &pAnInit->hadc1, sizeof(sAnalog.hadc1));
  memcpy(&sAnalog.hadc2, &pAnInit->hadc2, sizeof(sAnalog.hadc2));
  memcpy(&sAnalog.hadc3, &pAnInit->hadc3, sizeof(sAnalog.hadc3));
  memcpy(&sAnalog.hdma_adc1, &pAnInit->hdma_adc1, sizeof(sAnalog.hdma_adc1));
  memcpy(&sAnalog.hdma_adc2, &pAnInit->hdma_adc2, sizeof(sAnalog.hdma_adc2));
  memcpy(&sAnalog.hdma_adc3, &pAnInit->hdma_adc3, sizeof(sAnalog.hdma_adc3));
  return true;
}

/**************************************************************************************************\
* FUNCTION    Analog_getADCVal
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
uint16_t Analog_getADCVal(ADCSelect adc)
{
  ADC_ChannelConfTypeDef sConfig;
  sConfig.Rank = 1;
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  ADC_HandleTypeDef *adcNum;
  switch (adc)
  {
    case ADC_VREF_INTERNAL:   sConfig.Channel = ADC_CHANNEL_VREFINT; adcNum = &sAnalog.hadc1; break;
    case ADC_DOM1_INCURRENT:  sConfig.Channel = ADC_CHANNEL_1;       adcNum = &sAnalog.hadc1; break;
    case ADC_DOM2_INCURRENT:  sConfig.Channel = ADC_CHANNEL_2;       adcNum = &sAnalog.hadc1; break;
    
    case ADC_DOM0_VOLTAGE:    sConfig.Channel = ADC_CHANNEL_7;       adcNum = &sAnalog.hadc2; break;
    case ADC_DOM1_VOLTAGE:    sConfig.Channel = ADC_CHANNEL_14;      adcNum = &sAnalog.hadc2; break;
    case ADC_DOM2_VOLTAGE:    sConfig.Channel = ADC_CHANNEL_15;      adcNum = &sAnalog.hadc2; break;
    
    case ADC_DOM0_OUTCURRENT: sConfig.Channel = ADC_CHANNEL_4;       adcNum = &sAnalog.hadc3; break;
    case ADC_DOM1_OUTCURRENT: sConfig.Channel = ADC_CHANNEL_11;      adcNum = &sAnalog.hadc3; break;
    case ADC_DOM2_OUTCURRENT: sConfig.Channel = ADC_CHANNEL_13;      adcNum = &sAnalog.hadc3; break;
    default: return 0;
  }
  HAL_ADC_ConfigChannel(adcNum, &sConfig);
  HAL_ADC_Start(adcNum);
  return HAL_ADC_GetValue(adcNum);
}
