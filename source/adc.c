#include "stm32f2xx.h"
#include "adc.h"
#include "eeprom.h"
#include "util.h"

struct
{
  ADCControl adc1;
  ADCControl adc2;
  ADCControl adc3;
  boolean isBufferFull;
} sADC;

/**************************************************************************************************\
* FUNCTION    ADC_init
* DESCRIPTION Initializes the analog to digital converters to their default state
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void ADC_init(void)
{
  //ADC->CCR  = 0;
  ADC->CCR &= ADC_CCR_ADCPRE;  // ADC clk = PCLK2/2 = 30MHz
  ADC->CCR |= ADC_CCR_TSVREFE; // turn on the temperature sensor

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable peripheral clock for ADC1
    //ADC1->SQR1    = (1UL << 20); // 2 Conversions
    ADC1->SQR1    = (0UL << 20); // 1 Conversion
    ADC1->SQR2    = (0);
    //ADC1->SQR3    = (17UL << 0) | (1UL << 5); // Convert INTVREF + ADCIN_1
    ADC1->SQR3    = (17UL << 0); // Convert INTVREF
    ADC1->SMPR1   = (0x6DB6DB6D);
    ADC1->SMPR2   = (0x6DB6DB6D);
    ADC1->CR1     = (ADC_CR1_SCAN | ADC_CR1_EOCIE);
    ADC1->CR2     = (ADC_CR2_EOCS | ADC_CR2_CONT | ADC_CR2_ADON);
    ADC1->SR     &= (~(ADC_SR_OVR | ADC_SR_EOC));
    Util_fillMemory((uint8*)&sADC.adc1, sizeof(sADC.adc1), 0x00);


    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN; // Enable peripheral clock for ADC2
    //ADC2->SQR1    = (1UL << 20); // 2 Conversions
    ADC1->SQR1    = (0UL << 20); // 1 Conversion
    ADC2->SQR2    = (0);
    //ADC2->SQR3    = (6UL << 0) | (2UL << 5); // Convert EXTVREF + ADCIN2
    ADC2->SQR3    = (2UL << 0); // Convert ADCIN2
    ADC2->SMPR1   = (0x6DB6DB6D);
    ADC2->SMPR2   = (0x6DB6DB6D);
    ADC2->CR1     = (ADC_CR1_SCAN | ADC_CR1_EOCIE);
    ADC2->CR2     = (ADC_CR2_EOCS | ADC_CR2_CONT | ADC_CR2_ADON);
    ADC2->SR     &= (~(ADC_SR_OVR | ADC_SR_EOC));
    Util_fillMemory((uint8*)&sADC.adc2, sizeof(sADC.adc2), 0x00);

    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN; // Enable peripheral clock for ADC3
    //ADC3->SQR1    = (1UL << 20); // 2 Conversions
    ADC1->SQR1    = (0UL << 20); // 1 Conversion
    ADC3->SQR2    = (0);
    //ADC3->SQR3    = (0UL << 0) | (3UL << 5); // Convert BUSVOUT + ADCIN_3;
    ADC3->SQR3    = (3UL << 0); // Convert ADCIN_3;
    ADC3->SMPR1   = (0x6DB6DB6D);
    ADC3->SMPR2   = (0x6DB6DB6D);
    ADC3->CR1     = (ADC_CR1_SCAN | ADC_CR1_EOCIE);
    ADC3->CR2     = (ADC_CR2_EOCS | ADC_CR2_CONT | ADC_CR2_ADON);
    ADC3->SR     &= (~(ADC_SR_OVR | ADC_SR_EOC));
    Util_fillMemory((uint8*)&sADC.adc3, sizeof(sADC.adc3), 0x00);

  NVIC_EnableIRQ(ADC_IRQn);               /* enable ADC Interrupt             */
}

boolean ADC_isBufferFull(ADCPort port)
{
  switch (port)
  {
    case ADC_PORT1:
      return sADC.adc1.isBufferFull;
    case ADC_PORT2:
      return sADC.adc2.isBufferFull;
    case ADC_PORT3:
      return sADC.adc3.isBufferFull;
    default:
      return FALSE;
  }
}

ADCControl* ADC_getControlPtr(ADCPort a2d)
{
  if (a2d == A2D1)
    return &sADC.adc1;
  else if (a2d == A2D2)
    return &sADC.adc2;
  else if (a2d == A2D3)
    return &sADC.adc3;
  else
    return 0;
}

/*----------------------------------------------------------------------------
  start AD Conversion
 *----------------------------------------------------------------------------*/
void ADC_StartCnv(boolean adc1, boolean adc2, boolean adc3)
{
  if (adc1)
    ADC1->CR2 |= ADC_CR2_SWSTART;
  if (adc2)
    ADC2->CR2 |= ADC_CR2_SWSTART;
  if (adc3)
    ADC3->CR2 |= ADC_CR2_SWSTART;
}


/*----------------------------------------------------------------------------
  get converted AD value
 *----------------------------------------------------------------------------*/
uint16_t ADC_GetCnv(uint8_t adc)
{
  if (adc == 1)
  {
    while (!(ADC1->SR & ADC_SR_EOC)); // wait for completion
    return ADC1->DR;
  }
  else if (adc == 2)
  {
    while (!(ADC2->SR & ADC_SR_EOC)); // wait for completion
    return ADC2->DR;
  }
  else if (adc == 3)
  {
    while (!(ADC3->SR & ADC_SR_EOC)); // wait for completion
    return ADC3->DR;
  }
  else
    return 0;
}

uint16_t Main_getADC(uint8_t adc, uint8_t chan)
{
  uint16_t adcVal = 0;
  if (adc == 1)
  {
    ADC1->SQR3 = (ADC1->SQR3 & 0xC0000000) | (chan <<  0);
    ADC_StartCnv(TRUE, FALSE, FALSE);
    adcVal = ADC_GetCnv(1);
  }
  else if (adc == 2)
  {
    ADC2->SQR3 = (ADC2->SQR3 & 0xC0000000) | (chan <<  0);
    ADC_StartCnv(FALSE, TRUE, FALSE);
    adcVal = ADC_GetCnv(2);
  }
  else if (adc == 3)
  {
    ADC3->SQR3 = (ADC3->SQR3 & 0xC0000000) | (chan <<  0);
    ADC_StartCnv(FALSE, FALSE, TRUE);
    adcVal = ADC_GetCnv(3);
  }
  return adcVal;
}

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is done
 *----------------------------------------------------------------------------*/
void ADC_IRQHandler(void)
{
  uint16 timerCount = TIM2->CNT;
  if (ADC1->SR & ADC_SR_EOC)
  { // ADC1 EOC interrupt?
    if (!sADC.adc1.isBufferFull)
    {
      sADC.adc1.adcBuffer[0][sADC.adc1.bufIdx]   = ADC1->DR;
      sADC.adc1.adcBuffer[1][sADC.adc1.bufIdx++] = timerCount;
    }
    if (sADC.adc1.bufIdx >= ADC_BUFFER_SIZE)
    {
      sADC.adc1.isBufferFull = TRUE;
      sADC.adc1.bufIdx = 0;
    }
    ADC1->SR &= (~(ADC_SR_OVR | ADC_SR_EOC));
  }
  
  if (ADC2->SR & ADC_SR_EOC)
  { // ADC2 EOC interrupt?
    if (!sADC.adc2.isBufferFull)
    {
      sADC.adc2.adcBuffer[0][sADC.adc2.bufIdx]   = ADC2->DR;
      sADC.adc2.adcBuffer[1][sADC.adc2.bufIdx++] = timerCount;
    }
    if (sADC.adc2.bufIdx >= ADC_BUFFER_SIZE)
    {
      sADC.adc2.isBufferFull = TRUE;
      sADC.adc2.bufIdx = 0;
    }
    ADC2->SR &= (~(ADC_SR_OVR | ADC_SR_EOC));
  }
  
  if (ADC3->SR & ADC_SR_EOC)
  { // ADC3 EOC interrupt?
    if (!sADC.adc3.isBufferFull)
    {
      sADC.adc3.adcBuffer[0][sADC.adc3.bufIdx] = ADC3->DR;
      switch (EEPROM_getState())
      {
        case EEPROM_IDLE:
          sADC.adc3.adcBuffer[1][sADC.adc3.bufIdx++] = 1000;
          break;
        case EEPROM_WRITING:
          sADC.adc3.adcBuffer[1][sADC.adc3.bufIdx++] = 2000;
          break;
        case EEPROM_WAITING:
          sADC.adc3.adcBuffer[1][sADC.adc3.bufIdx++] = 3000;
          break;
        case EEPROM_READBACK:
          sADC.adc3.adcBuffer[1][sADC.adc3.bufIdx++] = 4000;
          break;
      }
    }
    if (sADC.adc3.bufIdx >= ADC_BUFFER_SIZE)
    {
      sADC.adc3.isBufferFull = TRUE;
      sADC.adc3.bufIdx = 0;
    }
    ADC3->SR &= (~(ADC_SR_OVR | ADC_SR_EOC));
  }
}

float Main_getInternalReferenceVoltage(void)
{
  float temp;
  ADC1->SQR3 = (ADC1->SQR3 & 0xC0000000) | (17UL <<  0);
  ADC_StartCnv(TRUE, FALSE, FALSE);
  temp  = ADC_GetCnv(1);
  temp *= 3.3 / 4095.0;
  return temp;
}

float Main_getInternalTemperature(void)
{
  float temp;
  ADC1->SQR3 = (ADC1->SQR3 & 0xC0000000) | (16UL <<  0);
  // V25c = .76V, slope = 2.5mV/degC
  temp  = Main_getADC(0, 17);
  temp *= 3.3 / 4095.0;
  temp  = temp -.76;
  temp *= .0025;
  return temp;
}

#define FILE_ID ADC_C
