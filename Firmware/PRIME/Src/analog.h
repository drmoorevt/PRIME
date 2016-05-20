#ifndef ANALOG_H
#define ANALOG_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef enum
{
  ADC_VREF_INTERNAL   = 0x00,
  ADC_DOM1_INCURRENT  = 0x01,
  ADC_DOM2_INCURRENT  = 0x02,
  ADC_DOM0_VOLTAGE    = 0x03,
  ADC_DOM1_VOLTAGE    = 0x04,
  ADC_DOM2_VOLTAGE    = 0x05,
  ADC_DOM0_OUTCURRENT = 0x06,
  ADC_DOM1_OUTCURRENT = 0x07,
  ADC_DOM2_OUTCURRENT = 0x08,
  ADC_SELECT_MAX      = 0x09,
} ADCSelect;

typedef struct
{
  ADC_HandleTypeDef hadc1;
  ADC_HandleTypeDef hadc2;
  ADC_HandleTypeDef hadc3;
  DMA_HandleTypeDef hdma_adc1;
  DMA_HandleTypeDef hdma_adc2;
  DMA_HandleTypeDef hdma_adc3;
} AnalogInit;

bool Analog_init(AnalogInit *pAnInit);
uint16_t Analog_getADCVal(ADCSelect adc);
double Analog_getADCVoltage(ADCSelect adc);

#endif
