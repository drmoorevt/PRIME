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

typedef enum
{
  TEST_IDLE     = 0,
  TEST_WAITING  = 1,
  TEST_RUNNING  = 3,
  TEST_COMPLETE = 4
} TestState;

enum
{
  TEST_RUN_COMMAND      = 0,
  TEST_STATUS_COMMAND   = 1,
  TEST_SENDDATA_COMMAND = 2,
  TEST_RESET_COMMAND    = 3
} TestCommand;

typedef struct
{
  uint8   channel;
  uint16  numSamples;
  boolean isSampling;
  __attribute__((aligned)) uint16 adcBuffer[TESTS_MAX_SAMPLES];
} Samples;

typedef struct
{
  uint16 headerBytes;  // The size of this struct
  uint16 timeScale;    // Time between samples in micro seconds
  uint16 bytesPerChan; // Number of bytes to expect per channel
  uint16 numChannels;  // Total number of channels
} TestHeader;

typedef struct
{
  uint8  chanNum;
  char   title[15]; // Padding this to a word length...
  double bitRes;
} ChanHeader;

static struct
{
  TestHeader testHeader;
  ChanHeader chanHeader[4];
  uint8  testToRun;
  TestState state;
  Samples adc1;
  Samples adc2;
  Samples adc3;
  Samples eeState;
  uint32  vAvg[TESTS_MAX_SAMPLES];
  uint32  iAvg[TESTS_MAX_SAMPLES];
  uint32  sAvg[TESTS_MAX_SAMPLES];
  struct
  {
    boolean portOpen;
    boolean receiving;
    boolean transmitting;
    __attribute__((aligned)) uint8 rxBuffer[256];
    __attribute__((aligned)) uint8 txBuffer[256];
  } comms;
} sTests;

uint16 Tests_test0(void);
uint16 Tests_test1(void);
uint16 Tests_test2(void);
uint16 Tests_test3(void);
uint16 Tests_test4(void);
uint16 Tests_test5(void);
uint16 Tests_test6(void);
uint16 Tests_test7(void);
uint16 Tests_test8(void);
uint16 Tests_test9(void);
uint16 Tests_test10(void);
uint16 Tests_test11(void);
uint16 Tests_test12(void);
uint16 Tests_test13(void);
uint8 Tests_parseTestCommand(void);
void Tests_notifyReceiveComplete(uint32 numBytes);
void Tests_notifyTransmitComplete(uint32 numBytes);
void Tests_notifyUnexpectedReceive(uint8 byte);
void Tests_sendBinaryResults(Samples *adcBuffer);
void Tests_sendHeaderInfo(void);

// A test function returns the number of data bytes and takes no parameters
typedef uint16 (*TestFunction)(void);

TestFunction testFunctions[] = { &Tests_test0,
                                 &Tests_test1,
                                 &Tests_test2,
                                 &Tests_test3,
                                 &Tests_test4,
                                 &Tests_test5,
                                 &Tests_test6,
                                 &Tests_test7,
                                 &Tests_test8,
                                 &Tests_test9,
                                 &Tests_test10,
                                 &Tests_test11,
                                 &Tests_test12,
                                 &Tests_test13 };

void Tests_init(void)
{
  AppCommConfig comm5 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sTests.comms.rxBuffer[0], &Tests_notifyReceiveComplete,
                           &sTests.comms.txBuffer[0], &Tests_notifyTransmitComplete,
                                                      &Tests_notifyUnexpectedReceive };
  Util_fillMemory(&sTests, sizeof(sTests), 0x00);
  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Enable comms domain
  UART_openPort(UART_PORT5, comm5);
  sTests.comms.portOpen = TRUE;
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

void Tests_notifySampleTrigger(void)
{
  if (sTests.eeState.numSamples < TESTS_MAX_SAMPLES)
    sTests.eeState.adcBuffer[sTests.eeState.numSamples++] = EEPROM_getState() * 1000;
}

/**************************************************************************************************\
* FUNCTION    Tests_notifyConversionComplete
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       This function is called upon interrupt indicating that numSamples have been taken
\**************************************************************************************************/
void Tests_notifyConversionComplete(uint8 chan, uint16 numSamples)
{
  switch (chan)
  {
    case ADC_Channel_1:
      sTests.adc1.channel = chan;
      sTests.adc1.isSampling = FALSE;
      sTests.adc1.numSamples = numSamples;
      break;
    case ADC_Channel_2:
      sTests.adc2.channel = chan;
      sTests.adc2.isSampling = FALSE;
      sTests.adc2.numSamples = numSamples;
      break;
    case ADC_Channel_3:
      sTests.adc3.channel = chan;
      sTests.adc3.isSampling = FALSE;
      sTests.adc3.numSamples = numSamples;
      break;
    default:
      break;
  }
  // should turn off the ADC timer here
}

// User sends a two-byte ASCII test number to run, respond with ACK or NAK
const uint8 runMessage[6]   = {'T','e','s','t','0','\n'};
// User sends a status request, respond with #bytes available for done or NAK for not done
const uint8 statMessage[7]  = {'S','t','a','t','u','s','\n'};
// User requests byte offset (uint16) into test buffer
const uint8 sendMessage[9]  = {'S','e','n','d','0','0','0','0','\n'};
// User sends a request to reset tests: flush the test buffer and reset state machine
const uint8 resetMessage[6] = {'R','e','s','e','t','\n'};

/******************************** Test START,SEND,RESET protocol **********************************\
 * User sends runMessage with two-byte ASCII test number to run, respond with ACK or NAK
 *   Ex: Request: 'Test(0x09)\n(crc16)' Response: '(ack/nak)(crc16)'
 * User sends status request
 *   Ex: Request: 'Status\n(crc16)' Response: '((uint16)#byteAvailable/nak)(crc16)'
 * User sends request for bytes in buffer
 *   Ex: Request: 'Send((uint16)offset)((uint16)numBytes)\n'
 *       Response '(data/nak)(crc16)
 *       Note: NAK is generated when offset+numBytes exceeds numBytesAvailable
 * User sends request to reset
 *   Ex: Request: 'Reset\n(crc16)' Response: '(ack)(crc16)'
 */

void Tests_run(void)
{
  // Testing
//  Tests_test0();
  switch (sTests.state)
  {
    case TEST_IDLE:            // Clear test data and setup listening for commands on the comm port
      sTests.testToRun = 0;
      Tests_receiveData(7, 0); // Pick up 8 bytes of test command
      sTests.state++;
      break;
    case TEST_WAITING:         // Waiting to receive 8 bytes on the comm port
      if (!sTests.comms.receiving)
      { // Received 8 bytes on the comm port, begin parsing and get the rest of the message
        sTests.testToRun = Tests_parseTestCommand();
        sTests.state = (sTests.testToRun > 0) ? TEST_RUNNING : TEST_IDLE;
      }
      break;
    case TEST_RUNNING:
      testFunctions[sTests.testToRun]();
      // Notify user that test is complete and to expect sizeofTestData bytes
      Tests_sendHeaderInfo();
      Tests_sendBinaryResults(&sTests.adc1);
      Tests_sendBinaryResults(&sTests.adc2);
      Tests_sendBinaryResults(&sTests.adc3);
      Tests_sendBinaryResults(&sTests.eeState);
      // Advance the state machine to data retrieval stage
      sTests.state = TEST_COMPLETE;
      break;
    case TEST_COMPLETE:
//      Tests_receiveData(11, 0); // Pick up 11 bytes of send command
      sTests.state = TEST_IDLE;
      break;
    default:
      break; // Error condition...
  }
}

uint8 Tests_parseTestCommand(void)
{
  if (sTests.comms.rxBuffer[0] == 'T' &&
      sTests.comms.rxBuffer[1] == 'e' &&
      sTests.comms.rxBuffer[2] == 's' &&
      sTests.comms.rxBuffer[3] == 't')
  {
    return sTests.comms.rxBuffer[4];
  }
  else
    return 0xFF;
}

void Tests_sendHeaderInfo(void)
{
  uint8 txBufOffset = 0,
                  i = 0;

  // Fill in the total number of bytes for the Test and Channel headers
  sTests.testHeader.headerBytes  =  sizeof(TestHeader);
  sTests.testHeader.headerBytes +=  sizeof(ChanHeader) * sTests.testHeader.numChannels;

  // Always send the test header
  Util_copyMemory( (uint8*)&sTests.testHeader,
                   (uint8*)&sTests.comms.txBuffer[txBufOffset],
                   sizeof(TestHeader));
  txBufOffset += sizeof(TestHeader);

  // Now send individual channel information
  for (i = 0; i < sTests.testHeader.numChannels; i++)
  {
    Util_copyMemory((uint8*)&sTests.chanHeader[i],
                    (uint8*)&sTests.comms.txBuffer[txBufOffset],
                    sizeof(ChanHeader));
    txBufOffset += sizeof(ChanHeader);
  }

  // Send the header out the UART
  Tests_sendData(txBufOffset);
  while(sTests.comms.transmitting);
}

void Tests_sendBinaryResults(Samples *adcBuffer)
{
  uint16 i = 0;
  for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  {
    sTests.comms.txBuffer[1] = adcBuffer->adcBuffer[i] >> 8;
    sTests.comms.txBuffer[0] = adcBuffer->adcBuffer[i];
    Tests_sendData(2);
    while(sTests.comms.transmitting); // wait to send out our test buffer
  }
}

void Tests_sendADCdata(void)
{
  uint16 i = 0, j = 0;

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
//  sTests.comms.txBuffer[i++] = '\r';
//  sTests.comms.txBuffer[i++] = '\n';
  sTests.comms.txBuffer[i++] = '\0';

  Tests_sendData(i);
  while(sTests.comms.transmitting); // wait to send out our test buffer
  
  for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  {  
    Tests_receiveData(2, 0);
    while(sTests.comms.receiving); // wait for two incoming bytes
    
    j = 0;
    Util_uint16ToASCII(sTests.adc1.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
    sTests.comms.txBuffer[j++] = ' ';
    Util_uint16ToASCII(sTests.adc2.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
    sTests.comms.txBuffer[j++] = ' ';
    Util_uint16ToASCII(sTests.adc3.adcBuffer[i], (char*)&sTests.comms.txBuffer[j]);
    j += 5;
//    sTests.comms.txBuffer[j++] = '\r';
//    sTests.comms.txBuffer[j++] = '\n';
    sTests.comms.txBuffer[j++] = '\0';
    
    Tests_sendData(j);
    while(sTests.comms.transmitting); // wait to send out our test buffer
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test0
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test0(void)
{
  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain

  while(1)
  {
    Time_delay(1000);
    Analog_setDomain(ENERGY_DOMAIN, FALSE);
    Time_delay(1000);
    Analog_setDomain(ENERGY_DOMAIN, TRUE);
  }

  while(1)
  {
    Util_spinWait(2000000);
    while ((GPIOC->IDR & 0x00008000) &&
          (GPIOC->IDR & 0x00004000) &&
          (GPIOC->IDR & 0x00002000));

    if ((GPIOC->IDR & 0x00008000) == 0)
    {
      Analog_setDomain(ANALOG_DOMAIN, TRUE);
      Analog_adjustDomain(EEPROM_DOMAIN, 0.6);
    }
    else if ((GPIOC->IDR & 0x00004000) == 0)
    {
      Analog_setDomain(ANALOG_DOMAIN, FALSE);
      Analog_adjustDomain(EEPROM_DOMAIN, 0.6);
    }
    else
    {

    }
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test1
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test1(void)
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

/**************************************************************************************************\
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
\**************************************************************************************************/
uint16 Tests_test2(void)
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


/**************************************************************************************************\
* FUNCTION    Tests_test3
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests the DAC
\**************************************************************************************************/
uint16 Tests_test3(void)
{
  float outVolts;
  DAC_init();
  while(1)
  {
    for (outVolts=0.0; outVolts < 3.3; outVolts+=.001)
      DAC_setVoltage(DAC_PORT2, outVolts);
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test4
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test4(void)
{
  Time_init();
  GPIO_init();
  DAC_init();
  while(1)
  {
    Util_spinWait(1);
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test5
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test5(void)
{
  Time_init();
  GPIO_init();
  DAC_init();
  while(1)
  {
    Util_spinWait(1);
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test6(void)
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

/**************************************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test7(void)
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

/**************************************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test8(void)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

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

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_selectChannel(EEPROM_DOMAIN, TRUE);

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  ADC_startSampleTimer(TIMER3, 6000);     // Start timer3 triggered ADCs at 100us sample rate

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test9
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test9(void)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_144Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
//  Analog_selectChannel(EEPROM_DOMAIN, TRUE);
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6);
  Time_delay(100); // Wait 10ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  sTests.adc1.isSampling = TRUE;
  sTests.adc2.isSampling = TRUE;
  sTests.adc3.isSampling = TRUE;
  ADC_startSampleTimer(TIMER3, 300);     // Start timer3 triggered ADCs at 5us sample rate

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0xFF);
  EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)0, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0xAA);
  EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)128, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x55);
  EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)256, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)384, 128);

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
  ADC_stopSampleTimer(TIMER3);

  // HACK: let EEPROM state be channel 22
  sTests.eeState.channel = 22;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = 5;
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = TESTS_MAX_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[3].chanNum  = sTests.eeState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Disable comms domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE);  // Enable SPI domain
  Time_delay(10); // Wait 10ms for domains to settle

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test10
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test10(void)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 1000ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  sTests.adc1.isSampling = TRUE;
  sTests.adc2.isSampling = TRUE;
  sTests.adc3.isSampling = TRUE;
  ADC_startSampleTimer(TIMER3, 300);     // Start timer3 triggered ADCs at 10us sample rate

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)0, 128);

  Time_delay(7);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  sTests.comms.rxBuffer[0] = 0xAA; // Changing up the first byte to test that this gets written
  EEPROM_writeEELowPower(&sTests.comms.rxBuffer[0], (uint8*)128, 128);

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
  ADC_stopSampleTimer(TIMER3);

  // HACK: let EEPROM state be channel 22
  sTests.eeState.channel = 22;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = 5;
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = TESTS_MAX_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[3].chanNum  = sTests.eeState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Enable comms domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE);  // Disable SPI domain
  Time_delay(10); // Wait 10ms for domains to settle

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test11
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts to
\**************************************************************************************************/
uint16 Tests_test11(void)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 10ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  sTests.adc1.isSampling = TRUE;
  sTests.adc2.isSampling = TRUE;
  sTests.adc3.isSampling = TRUE;
  ADC_startSampleTimer(TIMER3, 300);     // Start timer3 triggered ADCs at 10us sample rate

  Time_delay(7);
  Analog_adjustDomain(EEPROM_DOMAIN, 0.0); // Set domain voltage to maximum
  Time_delay(7);
  Analog_adjustDomain(EEPROM_DOMAIN, 3.3); // Set domain voltage to minimum
  Time_delay(7);
  Analog_adjustDomain(EEPROM_DOMAIN, 0.0); // Set domain voltage to maximum
  Time_delay(7);
  Analog_adjustDomain(EEPROM_DOMAIN, 3.3); // Set domain voltage to minimum
  Time_delay(7);
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to maximum
  

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
  ADC_stopSampleTimer(TIMER3);

  // HACK: let EEPROM state be channel 22
  sTests.eeState.channel = 22;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = 10;
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = TESTS_MAX_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[3].chanNum  = sTests.eeState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(COMMS_DOMAIN,  TRUE);   // Enable comms domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE);  // Disable SPI domain
  Time_delay(10); // Wait 10ms for domains to settle

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test12
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test uses the same technique as test11, but performs the test iteratively and
*             aggregates the results.
\**************************************************************************************************/
uint16 Tests_test12(void)
{
  uint16 i, j;
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 1000ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT3, adc3Config);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  Util_fillMemory(&sTests.vAvg, sizeof(sTests.vAvg), 0x00);
  Util_fillMemory(&sTests.iAvg, sizeof(sTests.iAvg), 0x00);
  Util_fillMemory(&sTests.sAvg, sizeof(sTests.sAvg), 0x00);
  Util_fillMemory(&sTests.adc2.adcBuffer, sizeof(sTests.adc2.adcBuffer), 0x00);
  for (i = 0; i < 512; i++)
  {
    // Using a variety of write bytes (i)
    Util_fillMemory(&sTests.comms.rxBuffer[0], 128, i);

    ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Get 4096 samples
    ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
    sTests.adc1.isSampling = TRUE;
    sTests.adc3.isSampling = TRUE;
    ADC_startSampleTimer(TIMER3, 300);     // Start timer3 triggered ADCs at 10us sample rate

    // write one page in each regular and low power mode
    EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)(128 * i), 128);
    Time_delay(7);
    EEPROM_writeEELowPower(&sTests.comms.rxBuffer[0], (uint8*)(128 * i), 128);
    Time_delay(7);

    // Complete the samples
    while(sTests.adc1.isSampling || sTests.adc3.isSampling);
    ADC_stopSampleTimer(TIMER3);

    // Aggregate the results into the voltage and current averages
    for (j = 0; j < TESTS_MAX_SAMPLES; j++)
    {
      sTests.vAvg[j] += sTests.adc1.adcBuffer[j];
      sTests.iAvg[j] += sTests.adc3.adcBuffer[j];
      sTests.sAvg[j] += sTests.eeState.adcBuffer[j];
    }
  }

  // Sample / Accumulate complete, divide by the number of samples
  for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  {
    sTests.adc1.adcBuffer[i]    = (uint16)(sTests.vAvg[i] / 512);
    sTests.adc3.adcBuffer[i]    = (uint16)(sTests.iAvg[i] / 512);
    sTests.eeState.adcBuffer[i] = (uint16)(sTests.sAvg[i] / 512);
  }

  // HACK: let EEPROM state be channel 22
  sTests.eeState.channel = 22;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = 10;
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = TESTS_MAX_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[3].chanNum  = sTests.eeState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Enable comms domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE);  // Disable SPI domain
  Time_delay(10); // Wait 10ms for domains to settle

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test13
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test uses the same technique as test11, but performs the test iteratively and
*             aggregates the results.
\**************************************************************************************************/
uint16 Tests_test13(void)
{
  uint16 i, j;
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc1Config.appSampleBuffer              = &sTests.adc1.adcBuffer[0];
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc2Config.appSampleBuffer              = &sTests.adc2.adcBuffer[0];
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_84Cycles;
  adc3Config.appSampleBuffer              = &sTests.adc3.adcBuffer[0];
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 1000ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT3, adc3Config);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  Util_fillMemory(&sTests.vAvg, sizeof(sTests.vAvg), 0x00);
  Util_fillMemory(&sTests.iAvg, sizeof(sTests.iAvg), 0x00);
  Util_fillMemory(&sTests.sAvg, sizeof(sTests.sAvg), 0x00);
  Util_fillMemory(&sTests.adc2.adcBuffer, sizeof(sTests.adc2.adcBuffer), 0x00);
  for (i = 0; i < 512; i++)
  {
    // Using a variety of write bytes (i)
    Util_fillMemory(&sTests.comms.rxBuffer[0], 128, i);

    ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Get 4096 samples
    ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
    sTests.adc1.isSampling = TRUE;
    sTests.adc3.isSampling = TRUE;
    ADC_startSampleTimer(TIMER3, 300);     // Start timer3 triggered ADCs at 10us sample rate

    // write one page in each regular and low power mode
    EEPROM_writeEE(&sTests.comms.rxBuffer[0], (uint8*)(128 * i), 128);
    Time_delay(7);
    EEPROM_writeEEXLP(&sTests.comms.rxBuffer[0], (uint8*)(128 * i), 128);
    Time_delay(7);

    // Complete the samples
    while(sTests.adc1.isSampling || sTests.adc3.isSampling);
    ADC_stopSampleTimer(TIMER3);

    // Aggregate the results into the voltage and current averages
    for (j = 0; j < TESTS_MAX_SAMPLES; j++)
    {
      sTests.vAvg[j] += sTests.adc1.adcBuffer[j];
      sTests.iAvg[j] += sTests.adc3.adcBuffer[j];
      sTests.sAvg[j] += sTests.eeState.adcBuffer[j];
    }
  }

  // Sample / Accumulate complete, divide by the number of samples
  for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  {
    sTests.adc1.adcBuffer[i]    = (uint16)(sTests.vAvg[i] / 512);
    sTests.adc3.adcBuffer[i]    = (uint16)(sTests.iAvg[i] / 512);
    sTests.eeState.adcBuffer[i] = (uint16)(sTests.sAvg[i] / 512);
  }

  // HACK: let EEPROM state be channel 22
  sTests.eeState.channel = 22;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = 10;
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = TESTS_MAX_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0);
  sTests.chanHeader[3].chanNum  = sTests.eeState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(COMMS_DOMAIN,  TRUE); // Enable comms domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE);  // Disable SPI domain
  Time_delay(10); // Wait 10ms for domains to settle

  return SUCCESS;
}
