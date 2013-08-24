#ifndef ADC_H
#define ADC_H

#include "types.h"

#define ADC_BUFFER_SIZE (4096)

typedef enum
{
  ADC_PORT1,
  ADC_PORT2,
  ADC_PORT3
} ADCPort;

typedef enum
{
    NONE,   // Manually via ADC_CR2_SWSTART
//    TIMER3  // Conversion occurs on Timer3 overflow
} ADCTriggerSource;

typedef struct
{
  uint16 vRef;
  uint16 bufIdx;
  boolean isBufferFull;
  uint16 adcBuffer[2][ADC_BUFFER_SIZE];
} ADCControl;

ADCControl* ADC_getControlPtr(ADCPort a2d);
void ADC_init(void);
void ADC_StartCnv(boolean adc1, boolean adc2, boolean adc3);
boolean ADC_isBufferFull(ADCPort port);
uint16_t Main_getADC(uint8_t adc, uint8_t chan);

#endif // ADC_H
