#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "uart.h"
#include "util.h"
#include "dac.h"
#include "time.h"

#define FILE_ID TESTS_C

/*****************************************************************************\
* FUNCTION    Tests_test0
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
/*
boolean Tests_test0(void)
{
  uint16_t i, adcResult[18];
  char adcResultChars[18][5];
  float adcVolts[18];
  char adcVoltsChars[18][5];
  float resolution;
  uint8_t adcNum;

  ADC_init();
  
  while (1)
  {
    Util_spinWait(2000000);
    
    if ((GPIOC->IDR & 0x00008000) == 0)
      adcNum = 1;
    else if ((GPIOC->IDR & 0x00004000) == 0)
      adcNum = 2;
    else if ((GPIOC->IDR & 0x00002000) == 0)
      adcNum = 3;
    
    UART_putChar(UART_PORT5, '\f');
    for (i=0; i < 18; i++)
    {
      adcResult[i] = Main_getADC(adcNum, i);
      adcResultChars[i][4] = ((adcResult[i] % 10     ) / 1     ) + 0x30;
      adcResultChars[i][3] = ((adcResult[i] % 100    ) / 10    ) + 0x30;
      adcResultChars[i][2] = ((adcResult[i] % 1000   ) / 100   ) + 0x30;
      adcResultChars[i][1] = ((adcResult[i] % 10000  ) / 1000  ) + 0x30;
      adcResultChars[i][0] = ((adcResult[i] % 100000 ) / 10000 ) + 0x30;
      if (i < 10)
        UART_putChar(UART_PORT5, i + 0x30);
      else
        UART_putChar(UART_PORT5, i + 0x41 - 10);
      UART_putChar(UART_PORT5, ':');
      UART_putChar(UART_PORT5, ' ');
      UART_putChar(UART_PORT5, adcResultChars[i][0]);
      UART_putChar(UART_PORT5, adcResultChars[i][1]);
      UART_putChar(UART_PORT5, adcResultChars[i][2]);
      UART_putChar(UART_PORT5, adcResultChars[i][3]);
      UART_putChar(UART_PORT5, adcResultChars[i][4]);
      UART_putChar(UART_PORT5, ' ');
      UART_putChar(UART_PORT5, '=');
      UART_putChar(UART_PORT5, ' ');
      
      resolution = 1.2 / (float)adcResult[6];
      adcVolts[i] = adcResult[i] * resolution * 10000;
      adcVoltsChars[i][4] = (((uint16_t)adcVolts[i] % 10     ) /  1    ) + 0x30;
      adcVoltsChars[i][3] = (((uint16_t)adcVolts[i] % 100    ) / 10    ) + 0x30;
      adcVoltsChars[i][2] = (((uint16_t)adcVolts[i] % 1000   ) / 100   ) + 0x30;
      adcVoltsChars[i][1] = (((uint16_t)adcVolts[i] % 10000  ) / 1000  ) + 0x30;
      adcVoltsChars[i][0] = (((uint16_t)adcVolts[i] % 100000 ) / 10000 ) + 0x30;
      UART_putChar(UART_PORT5, adcVoltsChars[i][0]);
      UART_putChar(UART_PORT5, '.');
      UART_putChar(UART_PORT5, adcVoltsChars[i][1]);
      UART_putChar(UART_PORT5, adcVoltsChars[i][2]);
      UART_putChar(UART_PORT5, adcVoltsChars[i][3]);
      UART_putChar(UART_PORT5, adcVoltsChars[i][4]);
      UART_putChar(UART_PORT5, 'v');
      
      if (i == 16)
      {
        UART_putChar(UART_PORT5, ' ');
        UART_putChar(UART_PORT5, '=');
        UART_putChar(UART_PORT5, ' ');
        adcVolts[i] /= 10000;
        adcVolts[i] -= .76;
        adcVolts[i] = 25 + (adcVolts[i] / .0025);
        adcVolts[i] *= 10000;
        adcVoltsChars[i][4] = (((uint32_t)adcVolts[i] % 10     ) /  1    ) + 0x30;
        adcVoltsChars[i][3] = (((uint32_t)adcVolts[i] % 100    ) / 10    ) + 0x30;
        adcVoltsChars[i][2] = (((uint32_t)adcVolts[i] % 1000   ) / 100   ) + 0x30;
        adcVoltsChars[i][1] = (((uint32_t)adcVolts[i] % 10000  ) / 1000  ) + 0x30;
        adcVoltsChars[i][0] = (((uint32_t)adcVolts[i] % 100000 ) / 10000 ) + 0x30;
        UART_putChar(UART_PORT5, adcVoltsChars[i][0]);
        UART_putChar(UART_PORT5, adcVoltsChars[i][1]);
        UART_putChar(UART_PORT5, '.');
        UART_putChar(UART_PORT5, adcVoltsChars[i][2]);
        UART_putChar(UART_PORT5, adcVoltsChars[i][3]);
        UART_putChar(UART_PORT5, adcVoltsChars[i][4]);
        UART_putChar(UART_PORT5, 'd');
        UART_putChar(UART_PORT5, 'e');
        UART_putChar(UART_PORT5, 'g');
        UART_putChar(UART_PORT5, 'C');
      }
      
      UART_putChar(UART_PORT5, '\r');
      UART_putChar(UART_PORT5, '\n');
    }
  }
}
*/

/*****************************************************************************\
* FUNCTION    Tests_test1
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
boolean Tests_test1(void)
{
	ADC_init();
  UART_init();
  Time_initTimer2();
  ADC_StartCnv(TRUE, TRUE, TRUE);
  while(1)
  {
    Util_spinWait(2000000);
    while ((GPIOC->IDR & 0x00008000) && 
          (GPIOC->IDR & 0x00004000) &&
          (GPIOC->IDR & 0x00002000));
    if ((GPIOC->IDR & 0x00008000) == 0)
      Analog_sampleDomain(MCU_DOMAIN);
    else if ((GPIOC->IDR & 0x00004000) == 0)
      Analog_sampleDomain(ANALOG_DOMAIN);
    else
      Analog_sampleDomain(EEPROM_DOMAIN);
  }
}

/*****************************************************************************\
* FUNCTION    Tests_test2
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       
* The goal of this test is to measure the current consumption of EEPROM during a complete write/flash erase/write cycle
* We begin by starting the timer 0 interrupt to tick every 1MHz = 1us = (Timer1@120MHz) = underflow every 120 ticks
*   The ISR should read/store the result of the previous ADC conversion and then begin another
*   The ISR should increment a counter indicating how many samples have occurred and stop before overflowing the buffer
*   The ISR should reset the timer value to 120 before exiting
* We will not use an interrupt to on the ADC conversion complete
* Next we 
\*****************************************************************************/
boolean Tests_test2(void)
{
  uint8 testBuffer[128];
  
  DAC_init();
//  Analog_setDomain(EEPROM_DOMAIN, TRUE);
//  Analog_adjustDomain(EEPROM_DOMAIN, 0.3);
  
  Analog_adjustDomain(ANALOG_DOMAIN, 0.4);
  
  Time_initTimer2();
  EEPROM_init();
  
  Util_fillMemory(testBuffer, 128, 0xA5);
  while(1)
  {
    EEPROM_writeEE(testBuffer, 0, 128);
    Util_spinWait(2000000);
  }
}

//Tests the DAC
boolean Tests_test3(void)
{
  float outVolts;
  DAC_init();
  while(1)
  {
    for (outVolts=0.0; outVolts < 3.3; outVolts+=.001)
      DAC_setVoltage(DAC_PORT2, outVolts);
  }
}

boolean Tests_test4(void)
{
  GPIO_init();
  DAC_init();
  Time_initTimer2();
  while(1)
  {
    Util_spinWait(1);
  }
}


void Tests_sendADCdata(void)
{/*
  uint16 i;
  char outChars[40];
  ADCControl* pAdc1 = ADC_getControlPtr(ADC_PORT1);
  ADCControl* pAdc2 = ADC_getControlPtr(ADC_PORT2);
  ADCControl* pAdc3 = ADC_getControlPtr(ADC_PORT3);
  FullDuplexPort* sp = UART_getPortPtr(UART_PORT5);
  
  i = 0;
  sp->tx.txBuffer[i++] = '\f';
  sp->tx.txBuffer[i++] = '\r';
  sp->tx.txBuffer[i++] = '\n';
  sp->tx.txBuffer[i++] = 'T';
  sp->tx.txBuffer[i++] = '1';
  sp->tx.txBuffer[i++] = ' ';
  sp->tx.txBuffer[i++] = 'V';
  sp->tx.txBuffer[i++] = 'r';
  sp->tx.txBuffer[i++] = ' ';
  sp->tx.txBuffer[i++] = 'T';
  sp->tx.txBuffer[i++] = '1';
  sp->tx.txBuffer[i++] = ' ';
  sp->tx.txBuffer[i++] = 'B';
  sp->tx.txBuffer[i++] = 'I';
  sp->tx.txBuffer[i++] = ' ';
  sp->tx.txBuffer[i++] = 'T';
  sp->tx.txBuffer[i++] = '1';
  sp->tx.txBuffer[i++] = ' ';
  sp->tx.txBuffer[i++] = 'B';
  sp->tx.txBuffer[i++] = 'O';
  sp->tx.txBuffer[i++] = '\r';
  sp->tx.txBuffer[i++] = '\n';
  
  UART_sendData(UART_PORT5, i);
  while (!UART_isTxBufIdle(UART_PORT5));
  
  for (i = 0; i < ADC_BUFFER_SIZE; i++)
  { 
    Util_uint16ToASCII(pAdc1->adcBuffer[1][i], &outChars[0]);
    outChars[5] = ' ';
    Util_uint16ToASCII(pAdc1->adcBuffer[0][i], &outChars[6]);
    outChars[11] = ' ';
    Util_uint16ToASCII(pAdc2->adcBuffer[1][i], &outChars[12]);
    outChars[17] = ' ';
    Util_uint16ToASCII(pAdc2->adcBuffer[0][i], &outChars[18]);
    outChars[23] = ' ';
    Util_uint16ToASCII(pAdc3->adcBuffer[1][i], &outChars[24]);
    outChars[29] = ' ';
    Util_uint16ToASCII(pAdc3->adcBuffer[0][i], &outChars[30]);
    outChars[35] = '\r';
    outChars[36] = '\n';
    Util_copyMemory((uint8*)&outChars[0], sp->tx.txBuffer, 37);
    UART_sendData(UART_PORT5, 37);
    while (!UART_isTxBufIdle(UART_PORT5));
  }
  */
}


/*****************************************************************************\
* FUNCTION    Tests_test5
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
/*
boolean Tests_test5(void)
{ 
  Analog_selectChannel(MCU_DOMAIN, TRUE);
  
  UART_init(UART_PORT5);
  Time_initTimer2();
  
  ADC_StartCnv(TRUE, TRUE, TRUE);
  while(!ADC_isBufferFull(ADC_PORT1) && !ADC_isBufferFull(ADC_PORT2) && !ADC_isBufferFull(ADC_PORT3));
  Tests_sendADCdata();
  
  while(1);
}
*/

/*****************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
boolean Tests_test6(void)
{ 
  uint8 testBuffer[128];
  ADC_init();
  DAC_init();
  
  Analog_setDomain(EEPROM_DOMAIN, TRUE); // Enable SPI domain********************
  Analog_adjustDomain(EEPROM_DOMAIN, 0.3);
  
  UART_init();
  Time_initTimer2();
  EEPROM_init();
    
  Util_fillMemory(testBuffer, 128, 0xA5); // try writing 0x00 next!
  
  TIM2->CNT = 0;
  
  ADC_StartCnv(TRUE, TRUE, TRUE);
  EEPROM_writeEE(testBuffer, 0, 128);
  while(!ADC_isBufferFull(ADC_PORT1) && !ADC_isBufferFull(ADC_PORT2) && !ADC_isBufferFull(ADC_PORT3));
  Tests_sendADCdata();
  
  while(1);
}

static struct
{
  boolean receiving;
  boolean transmitting;
  __attribute__((aligned)) uint8 rxBuffer[256];
  __attribute__((aligned)) uint8 txBuffer[256];
} sTests;

void Tests_init(void)
{
  Util_fillMemory(&sTests, sizeof(sTests), 0x00);
}

void Tests_notifyReceiveComplete(uint32 numBytes)
{
  sTests.receiving = FALSE;
}

void Tests_notifyTransmitComplete(uint32 numBytes)
{
  sTests.transmitting = FALSE;
}

void Tests_notifyUnexpectedReceive(uint8 byte)
{
  return;
}

void Tests_receiveData(uint16 numBytes, uint16 timeout)
{
  sTests.receiving = TRUE;
  UART_receiveData(UART_PORT5, numBytes);
}

void Tests_sendData(uint16 numBytes)
{
  sTests.transmitting = TRUE;
  UART_sendData(UART_PORT5, numBytes);
}

boolean Tests_test7(void)
{
  AppCommConfig comm5 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sTests.rxBuffer[0], &Tests_notifyReceiveComplete,
                           &sTests.txBuffer[0], &Tests_notifyTransmitComplete,
                                                &Tests_notifyUnexpectedReceive };
  uint8 testArray[18] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','\r','\n'};

  GPIO_init();

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  TRUE);  // Enable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain

  UART_init();
  Time_initTimer2();
  UART_openPort(UART_PORT5, comm5);

  Util_copyMemory(&testArray[0], &sTests.txBuffer[0], 18);

  while(1)
  {
    Tests_receiveData(2, 0);
    while(sTests.receiving); // wait for one incoming byte

    // Echo two bytes at a time
    sTests.txBuffer[0] = '\r';
    sTests.txBuffer[1] = '\n';
    Util_copyMemory(&sTests.rxBuffer[0], &sTests.txBuffer[2], 2);
    sTests.txBuffer[4] = '\r';
    sTests.txBuffer[5] = '\n';

    Tests_sendData(6);
    while(sTests.transmitting); // wait to send out our test buffer
  }
}
