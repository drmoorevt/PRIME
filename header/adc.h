#ifndef ADC_H
#define ADC_H

#include "types.h"

#define ADC_BUFFER_SIZE (4096)

typedef enum
{
  A2D1,
  A2D2,
  A2D3,
  NUM_A2DS
} A2D;

typedef struct
{
  uint16 vRef;
  uint16 bufIdx;
  boolean isBufferFull;
  uint16 adcBuffer[2][ADC_BUFFER_SIZE];
} ADCControl;

ADCControl* ADC_getControlPtr(A2D a2d);
void Main_adcConfig(A2D a2d);
void ADC_StartCnv(boolean adc1, boolean adc2, boolean adc3);
boolean ADC_isBufferFull(A2D a2d);
uint16_t Main_getADC(uint8_t adc, uint8_t chan);

#endif // ADC_H
