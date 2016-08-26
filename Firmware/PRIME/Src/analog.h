#ifndef ANALOG_H
#define ANALOG_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "time.h"

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

bool Analog_init(void);
uint16_t Analog_getADCVal(ADCSelect adc);
double Analog_getADCVoltage(ADCSelect adc, uint32_t numSamps);
double Analog_getADCCurrent(ADCSelect adc, const uint32_t numSamps);

typedef enum
{
  ADC_PORT1     = 0,
  ADC_PORT2     = 1,
  ADC_PORT3     = 2,
  NUM_ADC_PORTS = 3
} ADCPort;

typedef struct
{
  bool scan;
  bool continuous;
  uint8_t   numChannels;
  struct
  {
    uint8_t sampleTime;
    uint8_t chanNum;
  } chan[16];
  uint32_t numSamps;
} ADCConfig;

typedef struct
{
  ADCConfig adcConfig;  // Not using this yet, but ultimately allow apps to decide these params
  uint16_t *appSampleBuffer;
  void  (*appNotifyConversionComplete)(uint8_t, uint32_t);
} AppADCConfig;

void Analog_openPort(ADCSelect adcSelect, AppADCConfig appConfig);
void Analog_configureADC(ADCSelect adcSel, void *pDst, uint32_t numSamps);
boolean Analog_startSampleTimer(uint32_t sampRate);
boolean Analog_stopSampleTimer(void);

#endif
