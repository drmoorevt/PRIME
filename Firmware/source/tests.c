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

#define TESTS_MAX_SAMPLES (4096)
typedef struct
{
  uint8   channel;
  uint16  numSamples;
  boolean isSampling;
  __attribute__((aligned)) uint16 adcBuffer[TESTS_MAX_SAMPLES];
} Samples;

static struct
{
  Samples adc1;
  Samples adc2;
  Samples adc3;
  struct
  {
    boolean receiving;
    boolean transmitting;
    __attribute__((aligned)) uint8 rxBuffer[256];
    __attribute__((aligned)) uint8 txBuffer[256];
  } comms;
} sTests;

void Tests_init(void)
{
  Util_fillMemory(&sTests, sizeof(sTests), 0x00);
}

void Tests_notifyReceiveComplete(uint32 numBytes)
{
  sTests.comms.receiving = FALSE;
}

void Tests_notifyTransmitComplete(uint32 numBytes)
{
  sTests.comms.transmitting = FALSE;
}

void Tests_notifyUnexpectedReceive(uint8 byte)
{
  return;
}

void Tests_receiveData(uint16 numBytes, uint16 timeout)
{
  sTests.comms.receiving = TRUE;
  UART_receiveData(UART_PORT5, numBytes);
}

void Tests_sendData(uint16 numBytes)
{
  sTests.comms.transmitting = TRUE;
  UART_sendData(UART_PORT5, numBytes);
}

void Tests_notifyConversionComplete(uint8 chan, uint16 numSamples)
{
  switch (chan)
  {
    case 3:
      sTests.adc1.channel = chan;
      sTests.adc1.isSampling = FALSE;
      sTests.adc1.numSamples = numSamples;
      break;
    case 6:
      sTests.adc2.channel = chan;
      sTests.adc2.isSampling = FALSE;
      sTests.adc2.numSamples = numSamples;
      break;
    case 17:
      sTests.adc3.channel = chan;
      sTests.adc3.isSampling = FALSE;
      sTests.adc3.numSamples = numSamples;
      break;
    default:
      break;
  }
}

void Tests_sendADCdata(void)
{
  uint16 i = 0, j = 0;

  sTests.comms.txBuffer[i++] = '\f';
  sTests.comms.txBuffer[i++] = '\r';
  sTests.comms.txBuffer[i++] = '\n';
  sTests.comms.txBuffer[i++] = 'T';
  sTests.comms.txBuffer[i++] = '1';
  sTests.comms.txBuffer[i++] = ' ';
  sTests.comms.txBuffer[i++] = 'V';
  sTests.comms.txBuffer[i++] = 'r';
  sTests.comms.txBuffer[i++] = ' ';
  sTests.comms.txBuffer[i++] = 'T';
  sTests.comms.txBuffer[i++] = '1';
  sTests.comms.txBuffer[i++] = ' ';
  sTests.comms.txBuffer[i++] = 'B';
  sTests.comms.txBuffer[i++] = 'I';
  sTests.comms.txBuffer[i++] = ' ';
  sTests.comms.txBuffer[i++] = 'T';
  sTests.comms.txBuffer[i++] = '1';
  sTests.comms.txBuffer[i++] = ' ';
  sTests.comms.txBuffer[i++] = 'B';
  sTests.comms.txBuffer[i++] = 'O';
  sTests.comms.txBuffer[i++] = '\r';
  sTests.comms.txBuffer[i++] = '\n';

  Tests_sendData(6);
  while(sTests.comms.transmitting); // wait to send out our test buffer
  
  for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  {
    j = 0;
    Util_uint16ToASCII(sTests.adc1.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
    sTests.comms.txBuffer[j++] = ' ';
    Util_uint16ToASCII(sTests.adc2.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
    sTests.comms.txBuffer[j++] = ' ';
    Util_uint16ToASCII(sTests.adc3.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
    sTests.comms.txBuffer[j++] = '\r';
    sTests.comms.txBuffer[j++] = '\n';
    
    Tests_sendData(j);
    while(sTests.comms.transmitting); // wait to send out our test buffer
  }
}

/*****************************************************************************\
* FUNCTION    Tests_test1
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
boolean Tests_test1(void)
{
  Time_init();
	ADC_init();
  UART_init();
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
  Time_init();
  DAC_init();
//  Analog_setDomain(EEPROM_DOMAIN, TRUE);
//  Analog_adjustDomain(EEPROM_DOMAIN, 0.3);
  
  Analog_adjustDomain(ANALOG_DOMAIN, 0.4);

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
  Time_init();
  GPIO_init();
  DAC_init();
  while(1)
  {
    Util_spinWait(1);
  }
}

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
  Time_init();
  ADC_init();
  DAC_init();
  
  Analog_setDomain(EEPROM_DOMAIN, TRUE); // Enable SPI domain********************
  Analog_adjustDomain(EEPROM_DOMAIN, 0.3);
  
  UART_init();
  EEPROM_init();
    
  Util_fillMemory(testBuffer, 128, 0xA5); // @AGD: try writing 0x00 next!
  
  // START SAMPLER HERE
  EEPROM_writeEE(testBuffer, 0, 128);
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
  Tests_sendADCdata();
  
  while(1);
}

boolean Tests_test7(void)
{
  AppCommConfig comm5 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sTests.comms.rxBuffer[0], &Tests_notifyReceiveComplete,
                           &sTests.comms.txBuffer[0], &Tests_notifyTransmitComplete,
                                                      &Tests_notifyUnexpectedReceive };
  uint8 testArray[18] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','\r','\n'};

  Time_init();
  Analog_init();
  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  TRUE);  // Enable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain

  UART_init();
  UART_openPort(UART_PORT5, comm5);

  Util_copyMemory(&testArray[0], &sTests.comms.txBuffer[0], 18);

  while(1)
  {
    Tests_receiveData(2, 0);
    while(sTests.comms.receiving); // wait for one incoming byte

    // Echo two bytes at a time
    sTests.comms.txBuffer[0] = '\r';
    sTests.comms.txBuffer[1] = '\n';
    Util_copyMemory(&sTests.comms.rxBuffer[0], &sTests.comms.txBuffer[2], 2);
    sTests.comms.txBuffer[4] = '\r';
    sTests.comms.txBuffer[5] = '\n';

    Tests_sendData(6);
    while(sTests.comms.transmitting); // wait to send out our test buffer
  }
}

boolean Tests_test8(void)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};
  AppCommConfig comm5 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sTests.comms.rxBuffer[0], &Tests_notifyReceiveComplete,
                           &sTests.comms.txBuffer[0], &Tests_notifyTransmitComplete,
                                                      &Tests_notifyUnexpectedReceive };
  // ADC1 sampling intVref
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_Vrefint;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling extVref
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_6;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain voltage
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Time_init();
  Analog_init();

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_selectChannel(EEPROM_DOMAIN, TRUE);

  ADC_init();
  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  ADC_startSampleTimer(TIMER3, 6000);     // Start timer3 triggered ADCs at 100us sample rate

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);

  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Enable comms domain
  UART_init();
  UART_openPort(UART_PORT5, comm5);

  Tests_receiveData(2, 0);
  while(sTests.comms.receiving); // wait for two incoming bytes

  Tests_sendADCdata();

  while(1);
}




