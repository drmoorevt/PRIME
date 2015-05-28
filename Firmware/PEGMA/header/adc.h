#ifndef ADC_H
#define ADC_H

#include "types.h"
#include "time.h"  // Needed for the definition of HardTimer

// ENUMERATE THESE
#define ADC_SampleTime_3Cycles                    ((uint8_t)0x00)
#define ADC_SampleTime_15Cycles                   ((uint8_t)0x01)
#define ADC_SampleTime_28Cycles                   ((uint8_t)0x02)
#define ADC_SampleTime_56Cycles                   ((uint8_t)0x03)
#define ADC_SampleTime_84Cycles                   ((uint8_t)0x04)
#define ADC_SampleTime_112Cycles                  ((uint8_t)0x05)
#define ADC_SampleTime_144Cycles                  ((uint8_t)0x06)
#define ADC_SampleTime_480Cycles                  ((uint8_t)0x07)

#define ADC_Channel_0                               ((uint8_t)0x00)
#define ADC_Channel_1                               ((uint8_t)0x01)
#define ADC_Channel_2                               ((uint8_t)0x02)
#define ADC_Channel_3                               ((uint8_t)0x03)
#define ADC_Channel_4                               ((uint8_t)0x04)
#define ADC_Channel_5                               ((uint8_t)0x05)
#define ADC_Channel_6                               ((uint8_t)0x06)
#define ADC_Channel_7                               ((uint8_t)0x07)
#define ADC_Channel_8                               ((uint8_t)0x08)
#define ADC_Channel_9                               ((uint8_t)0x09)
#define ADC_Channel_10                              ((uint8_t)0x0A)
#define ADC_Channel_11                              ((uint8_t)0x0B)
#define ADC_Channel_12                              ((uint8_t)0x0C)
#define ADC_Channel_13                              ((uint8_t)0x0D)
#define ADC_Channel_14                              ((uint8_t)0x0E)
#define ADC_Channel_15                              ((uint8_t)0x0F)
#define ADC_Channel_16                              ((uint8_t)0x10)
#define ADC_Channel_17                              ((uint8_t)0x11)
#define ADC_Channel_18                              ((uint8_t)0x12)

#define ADC_Channel_TempSensor                      ((uint8_t)ADC_Channel_16)
#define ADC_Channel_Vrefint                         ((uint8_t)ADC_Channel_17)
#define ADC_Channel_Vbat                            ((uint8_t)ADC_Channel_18)

#define ADC_DR_OFFSET (0x4C)

typedef enum
{
  ADC_PORT1,
  ADC_PORT2,
  ADC_PORT3,
  NUM_ADC_PORTS
} ADCPort;

typedef struct
{
  boolean scan;
  boolean continuous;
  uint8   numChannels;
  struct
  {
    uint8 sampleTime;
    uint8 chanNum;
  } chan[16];
} ADCConfig;

typedef struct
{
  ADCConfig adcConfig;  // Not using this yet, but ultimately allow apps to decide these params
  uint16 *appSampleBuffer;
  void  (*appNotifyConversionComplete)(uint8, uint32);
} AppADCConfig;

void ADC_init(void);
void ADC_getSamples(ADCPort port, uint16 numSamples);
void ADC_openPort(ADCPort port, AppADCConfig adcConfig);
boolean ADC_startSampleTimer(HardTimer timer, uint16 reloadVal);
boolean ADC_stopSampleTimer(HardTimer timer);

#endif // ADC_H
