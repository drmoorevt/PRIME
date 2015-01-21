#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "crc.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "hih613x.h"
#include "sdcard.h"
#include "serialflash.h"
#include "sram.h"
#include "spi.h"
#include "time.h"
#include "usbvcp.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILE_ID TESTS_C

//#define TESTS_MAX_SAMPLES (10240)
//#define TESTS_MAX_SAMPLES (10060)
#define TESTS_MAX_SAMPLES (SRAM_NUM_CHANNEL_SAMPLES)

typedef enum
{
  HIH_CHANNEL_OVERLOAD = 21,
  EE_CHANNEL_OVERLOAD  = 22,
  SF_CHANNEL_OVERLOAD  = 23,
  SD_CHANNEL_OVERLOAD  = 24
} PeripheralChannels;

typedef enum
{
  TEST_IDLE     = 0,
  TEST_WAITING  = 1,
  TEST_RUNNING  = 3,
  TEST_COMPLETE = 4
} TestState;

typedef enum
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
//  uint16  adcBuffer[TESTS_MAX_SAMPLES];
  uint16  *pSampleBuffer;
} Samples;

typedef struct
{
  uint16 headerBytes;  // The size of this struct
  char   title[62];    // Title of this test
  uint16 timeScale;    // Time between samples in micro seconds
  uint32 bytesPerChan; // Number of bytes to expect per channel
  uint16 numChannels;  // Total number of channels
} TestHeader;

typedef struct
{
  uint8  chanNum;
  char   title[31];
  double bitRes;
} ChanHeader;

typedef struct
{
  uint32 preTestDelayUs;
  uint32 sampleRate;
  uint32 postTestDelayUs;
} CommonArgs;

typedef struct
{
  CommonArgs commonArgs      __attribute__((packed));
  EEPROMPowerProfile profile __attribute__((packed));
  uint8 writeBuffer[128];
  uint8 *pDest               __attribute__((packed));
  uint16 writeLength         __attribute__((packed));
} Test11Args;

typedef struct
{
  CommonArgs commonArgs      __attribute__((packed));
  SerialFlashPowerProfile profile __attribute__((packed));
  uint8 writeBuffer[128];
  uint8 *pDest               __attribute__((packed));
  uint16 writeLength         __attribute__((packed));
} Test12Args;

typedef struct
{
  CommonArgs commonArgs      __attribute__((packed));
  SDCardPowerProfile profile __attribute__((packed));
  uint8 writeBuffer[128];
  uint8 *pDest               __attribute__((packed));
  uint16 writeLength         __attribute__((packed));
  uint16 writeWait           __attribute__((packed));
} Test13Args;

typedef struct
{
  CommonArgs commonArgs;
  HIHPowerProfile profile;
  boolean measure;
  boolean read;
  boolean convert;
} Test14Args;

typedef union
{
  Test11Args test11Args;
  Test12Args test12Args;
  Test13Args test13Args;
  Test14Args test14Args;
  uint8 asBytes[256];
} TestArgs;

static struct
{
  TestHeader testHeader;
  ChanHeader chanHeader[4];
  uint8  testToRun;
  uint8  argCount;
  TestArgs testArgs;
  TestState state;
  uint32  powerProfile;
  volatile Samples adc1;
  volatile Samples adc2;
  volatile Samples adc3;
  volatile Samples periphState;
  uint32 (*getPeriphState)(void);
  volatile struct
  {
    uint32 bytesReceived;
    uint32 bytesToReceive;
    boolean portOpen;
    boolean receiving;
    boolean rxTimeout;
    boolean transmitting;
    __attribute__((aligned)) uint8 rxBuffer[256];
    __attribute__((aligned)) uint8 txBuffer[256];
  } comms;
} sTests;

uint16 Tests_test00(void *pArgs);
uint16 Tests_test01(void *pArgs);
uint16 Tests_test02(void *pArgs);
uint16 Tests_test03(void *pArgs);
uint16 Tests_test04(void *pArgs);
uint16 Tests_test05(void *pArgs);
uint16 Tests_test06(void *pArgs);
uint16 Tests_test07(void *pArgs);
uint16 Tests_test08(void *pArgs);
uint16 Tests_test09(void *pArgs);
uint16 Tests_test10(void *pArgs);
uint16 Tests_test11(void *pArgs);
uint16 Tests_test12(void *pArgs);
uint16 Tests_test13(void *pArgs);
uint16 Tests_test14(void *pArgs);
uint16 Tests_test15(void *pArgs);
uint16 Tests_test16(void *pArgs);
uint16 Tests_test17(void *pArgs);

void Tests_sendData(uint16 numBytes);
void Tests_sendBinaryResults(Samples *adcBuffer);
boolean Tests_sendHeaderInfo(void);

void Tests_notifyCommsEvent(USBVCPEvent event, uint32 arg)
{
  switch (event)
  {
    case USBVCP_EVENT_RX_COMPLETE:
      USBVCP_stopReceive();
      sTests.comms.receiving = FALSE;
      sTests.comms.bytesReceived = sTests.comms.bytesToReceive;
      break;
    case USBVCP_EVENT_RX_TIMEOUT:
      sTests.comms.bytesReceived = arg;
      sTests.comms.receiving = FALSE;
      sTests.comms.rxTimeout = TRUE;
      break;
    case USBVCP_EVENT_RX_INTERRUPT:
      break;
    case USBVCP_EVENT_TX_COMPLETE:
      sTests.comms.transmitting = FALSE;
      break;
    default:
      break;
  }
}

// A test function returns the number of data bytes and takes argc/argv parameters
typedef uint16 (*TestFunction)(void *pArgs);

TestFunction testFunctions[] = { &Tests_test00,
                                 &Tests_test01,
                                 &Tests_test02,
                                 &Tests_test03,
                                 &Tests_test04,
                                 &Tests_test05,
                                 &Tests_test06,
                                 &Tests_test07,
                                 &Tests_test08,
                                 &Tests_test09,
                                 &Tests_test10,
                                 &Tests_test11,
                                 &Tests_test12,
                                 &Tests_test13,
                                 &Tests_test14,
                                 &Tests_test15,
                                 &Tests_test16,
                                 &Tests_test17, };

/**************************************************************************************************\
* FUNCTION    Tests_init
* DESCRIPTION Initializes the communications interface and sets the domain voltages to begin tests
* PARAMETERS  numBytes: The number of bytes to wait for
*             timeout: The maximum amount time in milliseconds to wait for the data
* RETURNS     Nothing (uses callbacks via the USBVCP notifyXXX mechanism)
* NOTES       If timeout is 0 then the function can block forever
\**************************************************************************************************/
void Tests_init(void)
{
  USBVCPCommConfig usbComm = {(uint8 *)&sTests.comms.rxBuffer[0], (uint8 *)&sTests.comms.txBuffer[0],
                              &Tests_notifyCommsEvent };
  Util_fillMemory(&sTests, sizeof(sTests), 0x00);
  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Enable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  USBVCP_openPort(usbComm);
  sTests.comms.portOpen = TRUE;
  
  sTests.adc1.pSampleBuffer        = &GPSRAM->adc.samples[0][0];
  sTests.adc2.pSampleBuffer        = &GPSRAM->adc.samples[1][0];
  sTests.adc3.pSampleBuffer        = &GPSRAM->adc.samples[2][0];
  sTests.periphState.pSampleBuffer = &GPSRAM->adc.samples[3][0];
}

/**************************************************************************************************\
* FUNCTION    Tests_receiveData
* DESCRIPTION Will wait for either the specified number of bytes or for the timeout to expire
* PARAMETERS  numBytes: The number of bytes to wait for
*             timeout: The maximum amount time in milliseconds to wait for the data
* RETURNS     Nothing (uses callbacks via the USBVCP notifyXXX mechanism)
* NOTES       If timeout is 0 then the function can block forever
\**************************************************************************************************/
void Tests_receiveData(uint32 numBytes, uint32 timeout)
{
  /* The following spiVoltage variable and code executed while waiting for bytes allows the user to
     adjust the SPI voltage up, down and centered about 3.3V by pressing the push-buttons. This is
     really only applicable after startup and while waiting for tests to begin -- DON'T HOLD BUTTONS
     DURING TESTS */
  static double spiVoltage = 3.22734099999;
  
  sTests.comms.receiving = TRUE;
  sTests.comms.rxTimeout = FALSE;
  sTests.comms.bytesReceived = 0;
  sTests.comms.bytesToReceive = numBytes;
  USBVCP_receive(numBytes, timeout, TRUE);
  while(sTests.comms.receiving)
  {
    if ((GPIOC->IDR & 0x0000E000) != 0x0000E000)
    {
      Time_delay(100);
      if (!(GPIOC->IDR & 0x00008000))
        spiVoltage -= 0.000001;
      if (!(GPIOC->IDR & 0x00004000))
        spiVoltage  = 3.3000000;
      if (!(GPIOC->IDR & 0x00002000))
        spiVoltage += 0.000001;
      Analog_setDomain(SPI_DOMAIN, TRUE, spiVoltage);
      sprintf((char *)&sTests.comms.txBuffer[0], "%f\r\n", spiVoltage);
      Tests_sendData(10);
    }
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_sendData
* DESCRIPTION Sends the specified number of bytes out the USBVCP configured for tests
* PARAMETERS  numBytes: The number of bytes from the txBuffer to send out
* RETURNS     Nothing (uses callbacks via the USBVCP notifyXXX mechanism)
\**************************************************************************************************/
void Tests_sendData(uint16 numBytes)
{
  sTests.comms.transmitting = TRUE;
  USBVCP_send((uint8 *)&sTests.comms.txBuffer[0], numBytes);
  while(sTests.comms.transmitting);
}

/**************************************************************************************************\
* FUNCTION    Tests_getTestToRun
* DESCRIPTION Function returns only when a valid packet initiating a test has been received
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
uint8 Tests_getTestToRun(void)
{
  const uint32 MIN_PACKET_SIZE = 8;
  boolean hasValidPacket = FALSE;
  uint8 testToRun = 0, argCount = 0;
  uint32 size;

  do
  {
    // Grab the test execution packet
    Tests_receiveData(sizeof(sTests.comms.rxBuffer), 10); // With either icTimeout or txComplete
    if ((sTests.comms.rxBuffer[0] != 'T') ||
        (sTests.comms.rxBuffer[1] != 'e') ||
        (sTests.comms.rxBuffer[2] != 's') ||
        (sTests.comms.rxBuffer[3] != 't') ||
        (sTests.comms.rxBuffer[4] >=  17) ||
        (sTests.comms.bytesReceived < MIN_PACKET_SIZE))
      continue;
    testToRun = sTests.comms.rxBuffer[4];
    argCount  = sTests.comms.rxBuffer[5];
    size = MIN_PACKET_SIZE + argCount;
    hasValidPacket = (0x0000 == CRC_crc16(0x0000, CRC16_POLY_CCITT_STD, 
                      (uint8 *)&sTests.comms.rxBuffer, size));
  } while (!hasValidPacket);

  Util_copyMemory((uint8 *)&sTests.comms.rxBuffer[6], sTests.testArgs.asBytes, argCount);
  sTests.argCount = argCount;
  return testToRun;
}

/**************************************************************************************************\
* FUNCTION    Tests_notifySampleTrigger
* DESCRIPTION Function is called upon interrupt indicating that sample trigger (TMR3) has occurred
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Tests_notifySampleTrigger(void)
{
  if (FALSE == sTests.periphState.isSampling)
    return;
  if (sTests.periphState.numSamples++ < TESTS_MAX_SAMPLES)
    *sTests.periphState.pSampleBuffer++ = sTests.getPeriphState();
//    sTests.periphState.adcBuffer[sTests.periphState.numSamples++] = sTests.getPeriphState();
  else
    sTests.periphState.isSampling = FALSE;
}

/**************************************************************************************************\
* FUNCTION    Tests_notifyConversionComplete
* DESCRIPTION This function is called upon interrupt indicating that numSamples have been taken
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Tests_notifyConversionComplete(uint8 chan, uint32 numSamples)
{
  switch (chan)
  {
    case ADC_Channel_1:
      sTests.adc1.channel    = chan;
      sTests.adc1.isSampling = FALSE;
      break;
    case ADC_Channel_2:
      sTests.adc2.channel    = chan;
      sTests.adc2.isSampling = FALSE;
      break;
    case ADC_Channel_3:
      sTests.adc3.channel    = chan;
      sTests.adc3.isSampling = FALSE;
      break;
    default:
      break;
  }
  // should turn off the ADC timer here
}

// User sends a two-byte ASCII test number to run, respond with ACK or NAK
uint8 runMessage[6]   = {'T','e','s','t','0','\n'};
// User sends a status request, respond with #bytes available for done or NAK for not done
const uint8 statMessage[7]  = {'S','t','a','t','u','s','\n'};
// User requests byte offset (uint16) into test buffer
const uint8 sendMessage[9]  = {'S','e','n','d','0','0','0','0','\n'};
// User sends a request to reset tests: flush the test buffer and reset state machine
const uint8 resetMessage[6] = {'R','e','s','e','t','\n'};

/**************************************************************************************************\
* FUNCTION    Tests_runTest14
* DESCRIPTION Streams temperature and humidity information out the VCP
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       The delay for this function is dominated by the sampleRate, test measurements are
              occurring. You should let USB stabilize before entering this function.
\**************************************************************************************************/
void Tests_runTest14(void)
{
  uint16 testBuffer[20];
  double temperature, humidity;
  sTests.testArgs.test14Args.commonArgs.preTestDelayUs  = 100;
  sTests.testArgs.test14Args.commonArgs.sampleRate      = 1;
  sTests.testArgs.test14Args.commonArgs.postTestDelayUs = 100;
  sTests.testArgs.test14Args.profile = HIH_PROFILE_STANDARD;
  sTests.testArgs.test14Args.convert = TRUE;
  sTests.testArgs.test14Args.measure = TRUE;
  sTests.testArgs.test14Args.read = TRUE;
  Tests_test14(&sTests.testArgs);
  temperature = HIH613X_getTemperature();
  humidity = HIH613X_getHumidity();
  Util_copyMemory((uint8 *)&GPSRAM->adc.samples[0][0], (uint8 *)testBuffer, 20);
//  Tests_sendData(sprintf((char *)sTests.comms.txBuffer, "Temperature: %f, Humidity: %f\r\n", temperature, humidity));
}

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
 \*************************************************************************************************/
void Tests_run(void)
{
  //sTests.testArgs.test13Args.commonArgs.preTestDelayUs  = 1000;
  //sTests.testArgs.test13Args.commonArgs.sampleRate      = 100;
  //sTests.testArgs.test13Args.commonArgs.postTestDelayUs = 1000;
  //sTests.testArgs.test13Args.profile = SDCARD_PROFILE_STANDARD;
  //Util_fillMemory(sTests.testArgs.test11Args.writeBuffer, 64, 0xAA);
  //sTests.testArgs.test13Args.pDest = 0;
  //sTests.testArgs.test13Args.writeLength = 128;
  //while (1)
  //  Tests_test13(&sTests.testArgs);
  //while(1)
  //{
  //  Tests_receiveData(1, 1000);
  //  if (sTests.comms.bytesReceived > 0)
  //  {
  //    sTests.comms.txBuffer[0] = sTests.comms.rxBuffer[0];
  //    Tests_sendData(1);
  //  }
  //}
  //Util_copyMemory(&runMessage[0], &sTests.comms.txBuffer[0], sizeof(runMessage));
  //while (1)
  //{
  //  Tests_sendData(6);
  //}
  
  // SDCard_test();
//  Time_delay(1000*1000*5); // Let USB synchronize for 5s

//  while (1)
//    Tests_runTest14();
  
  switch (sTests.state)
  {
    case TEST_IDLE:  // Clear test data and setup listening for commands on the comm port
      sTests.testToRun = Tests_getTestToRun();
      sTests.state = TEST_WAITING;
      break;
    case TEST_WAITING:
      sTests.state = TEST_RUNNING; // No need to wait
      break;
    case TEST_RUNNING:
      testFunctions[sTests.testToRun](&sTests.testArgs);
      // Notify user that test is complete and to expect sizeofTestData bytes
      if (Tests_sendHeaderInfo())
      {
        while ((0x88 != sTests.comms.rxBuffer[0]) &&  // Binary result ACK
               (0x54 != sTests.comms.rxBuffer[0]))    // Test reset request
        {
          Tests_sendBinaryResults((Samples *)&sTests.adc1);
          Tests_sendBinaryResults((Samples *)&sTests.adc2);
          Tests_sendBinaryResults((Samples *)&sTests.adc3);
          Tests_sendBinaryResults((Samples *)&sTests.periphState);
          Tests_receiveData(1, 0);
        }
      }
      // Advance the state machine to data retrieval stage
      sTests.state = TEST_COMPLETE;
      break;
    case TEST_COMPLETE:
      sTests.state = TEST_IDLE;
      break;
    default:
      break; // Error condition...
  }
}

boolean Tests_sendHeaderInfo(void)
{
  uint8 i, txBufOffset = 0;
  boolean tfAck = FALSE;
  uint16 crc;

  // Fill in the total number of bytes for the Test and Channel headers
  sTests.testHeader.headerBytes  =  sizeof(TestHeader);
  sTests.testHeader.headerBytes +=  sizeof(ChanHeader) * sTests.testHeader.numChannels;
  sTests.testHeader.headerBytes +=  sizeof(crc);

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
  crc = CRC_crc16(0x0000, CRC16_POLY_CCITT_STD, (uint8 *)&sTests.comms.txBuffer, txBufOffset);
  Util_copyMemory((uint8 *)&crc, (uint8 *)&sTests.comms.txBuffer[txBufOffset], sizeof(crc));
  txBufOffset += 2;

  while (FALSE == tfAck)
  {
    // Send the header + CRC out the VCP
    Tests_sendData(txBufOffset);
    Tests_receiveData(1, 0); // Wait for the ack
    if (sTests.comms.rxBuffer[0] == 0x54) // reset attempt
      return FALSE;
    tfAck = (0x11 == sTests.comms.rxBuffer[0]);
  }
  return tfAck;
}

void Tests_sendBinaryResults(Samples *adcBuffer)
{
  uint32 bytesToTransmit, bytesLeft;
  uint8 *pData = (uint8 *)adcBuffer->pSampleBuffer;
  for (bytesLeft = SRAM_NUM_CHANNEL_SAMPLES * 2; bytesLeft > 0; bytesLeft -= bytesToTransmit)
  {
    bytesToTransmit = MIN(sizeof(sTests.comms.txBuffer), bytesLeft);
    Util_copyMemory(pData, (uint8 *)&sTests.comms.txBuffer[0], bytesToTransmit);
    Tests_sendData(bytesToTransmit);
    pData+= bytesToTransmit;
  }
  while(sTests.comms.transmitting);
}

/**************************************************************************************************\
* FUNCTION    Tests_setupSPITests
* DESCRIPTION Common setup actions for SPI tests
* PARAMETERS  reloadVal -- the sampling rate for SPI tests
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
static void Tests_setupSPITests(PeripheralChannels periph, uint32 sampleRate, double initVoltage)
{
  AppADCConfig adc1Config = {0};
  AppADCConfig adc2Config = {0};
  AppADCConfig adc3Config = {0};

  Analog_setDomain(SPI_DOMAIN, TRUE, initVoltage);  // Set domain voltage to ideal for IDLE state
  Time_delay(10000); // The SF chip has a power on reset timer requiring 10ms max to finish

  // ADC1 sampling domain voltage
  adc1Config.adcConfig.scan               = FALSE;
  adc1Config.adcConfig.continuous         = FALSE;
  adc1Config.adcConfig.numChannels        = 1;
  adc1Config.adcConfig.chan[0].chanNum    = ADC_Channel_1;
  adc1Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_15Cycles;
  adc1Config.appSampleBuffer              = sTests.adc1.pSampleBuffer;
  adc1Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC2 sampling domain input current
  adc2Config.adcConfig.scan               = FALSE;
  adc2Config.adcConfig.continuous         = FALSE;
  adc2Config.adcConfig.numChannels        = 1;
  adc2Config.adcConfig.chan[0].chanNum    = ADC_Channel_2;
  adc2Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_15Cycles;
  adc2Config.appSampleBuffer              = sTests.adc2.pSampleBuffer;
  adc2Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // ADC3 sampling domain output current
  adc3Config.adcConfig.scan               = FALSE;
  adc3Config.adcConfig.continuous         = FALSE;
  adc3Config.adcConfig.numChannels        = 1;
  adc3Config.adcConfig.chan[0].chanNum    = ADC_Channel_3;
  adc3Config.adcConfig.chan[0].sampleTime = ADC_SampleTime_15Cycles;
  adc3Config.appSampleBuffer              = sTests.adc3.pSampleBuffer;
  adc3Config.appNotifyConversionComplete  = &Tests_notifyConversionComplete;

  // Prepare data structures for retrieval
  sTests.testHeader.timeScale = sampleRate; // in microseconds (60MHz clock)
  sTests.testHeader.numChannels = 4;
  sTests.testHeader.bytesPerChan = SRAM_NUM_CHANNEL_SAMPLES * 2;
  sTests.chanHeader[0].chanNum  = adc1Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[0].bitRes   = (3.3 / 4096.0) * 2;  // Voltage measurements are div2
  sTests.chanHeader[1].chanNum  = adc2Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[1].bitRes   = (3.3 / 4096.0) * (1000.0 / 26.0); // (Gain = 20, R=1.3)
  sTests.chanHeader[2].chanNum  = adc3Config.adcConfig.chan[0].chanNum;
  sTests.chanHeader[2].bitRes   = (3.3 / 4096.0) * (1000.0 / 26.0); // (Gain = 20, R=1.3)
  sTests.chanHeader[3].chanNum  = sTests.periphState.channel;
  switch (sTests.testToRun)
  {
    case 11: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / EEPROM_STATE_MAX)       / 4096.0); break;
    case 12: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / SERIAL_FLASH_STATE_MAX) / 4096.0); break;
    case 13: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / SDCARD_STATE_MAX)       / 4096.0); break;
    case 14: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / HIH_STATE_MAX)          / 4096.0); break;
    default: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / 5)                      / 4096.0); break;
  }
  
  // Disable all interrupts (except for the adc trigger which will be enabled last)
  DISABLE_SYSTICK_INTERRUPT();
  NVIC_DisableIRQ(OTG_FS_IRQn);
  NVIC_DisableIRQ(USART3_IRQn);
  NVIC_DisableIRQ(UART4_IRQn);
  NVIC_DisableIRQ(UART5_IRQn);

  ADC_openPort(ADC_PORT1, adc1Config);        // Initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  switch (periph)
  {
    case HIH_CHANNEL_OVERLOAD: sTests.getPeriphState = HIH613X_getStateAsWord;     break;
    case EE_CHANNEL_OVERLOAD:  sTests.getPeriphState = EEPROM_getStateAsWord;      break;
    case SF_CHANNEL_OVERLOAD:  sTests.getPeriphState = SerialFlash_getStateAsWord; break;
    case SD_CHANNEL_OVERLOAD:  sTests.getPeriphState = SDCard_getStateAsWord;      break;
    default: break;
  }
  sTests.periphState.channel = periph;        // For sorting out the state of the peripheral
  sTests.periphState.isSampling = TRUE;
  sTests.periphState.numSamples = 0;                  // incremented on each sample
  
  sTests.adc1.numSamples = SRAM_NUM_CHANNEL_SAMPLES;  // untouched by test
  sTests.adc2.numSamples = SRAM_NUM_CHANNEL_SAMPLES;
  sTests.adc3.numSamples = SRAM_NUM_CHANNEL_SAMPLES;
  ADC_getSamples(ADC_PORT1, SRAM_NUM_CHANNEL_SAMPLES); // Will notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, SRAM_NUM_CHANNEL_SAMPLES);
  ADC_getSamples(ADC_PORT3, SRAM_NUM_CHANNEL_SAMPLES);
  sTests.adc1.isSampling = TRUE;
  sTests.adc2.isSampling = TRUE;
  sTests.adc3.isSampling = TRUE;
  // Start timer3 triggered ADCs at reloadVal = (sampleRate * 60), timer is at 60MHz
  ADC_startSampleTimer(TIME_HARD_TIMER_TIMER3, sampleRate * 60);
}

/**************************************************************************************************\
* FUNCTION    Tests_teardownSPITests
* DESCRIPTION Common teardown actions for SPI tests
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
static void Tests_teardownSPITests(boolean testPassed)
{
  Analog_setDomain(SPI_DOMAIN, FALSE, 3.3);  // Immediately Disable the SPI domain

  // Then wait for any ongoing tests to complete
  while(sTests.adc1.isSampling || 
        sTests.adc2.isSampling || 
        sTests.adc3.isSampling || 
        sTests.periphState.isSampling);
  ADC_stopSampleTimer(TIME_HARD_TIMER_TIMER3);
  // Enable all previous interrupts
  ENABLE_SYSTICK_INTERRUPT();
  NVIC_EnableIRQ(OTG_FS_IRQn);
  NVIC_EnableIRQ(USART3_IRQn);
  NVIC_EnableIRQ(UART4_IRQn);
  NVIC_EnableIRQ(UART5_IRQn);

  // Return domain to initial state
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Disable the SPI domain

  sprintf(sTests.chanHeader[0].title, "Domain Voltage (V)");
  sprintf(sTests.chanHeader[1].title, "Domain Input Current (mA)");
  sprintf(sTests.chanHeader[2].title, "Domain Output Current (mA)");
  
  switch (sTests.testToRun)
  {
    case 11:
      sprintf(sTests.chanHeader[3].title, "EEPROM State");
      switch (sTests.powerProfile)
      {
        case 0:
          if (testPassed)
            sprintf(sTests.testHeader.title, "EEPROM PROFILE STANDARD Passed");
          else
            sprintf(sTests.testHeader.title, "EEPROM PROFILE STANDARD Failed");
          break;
        case 1:
          if (testPassed)
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 25VIW Passed");
          else
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 25VIW Failed");
          break;
        case 2:
          if (testPassed)
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 18VIW Passed");
          else
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 18VIW Failed");
          break;/*
        case 3:
          if (testPassed)
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 14VIW Passed");
          else
            sprintf(sTests.testHeader.title, "EEPROM PROFILE 14VIW Failed");
          break;*/
        default: break;
      }
      break;
    case 12:
      sprintf(sTests.chanHeader[3].title, "SerialFlash State");
      switch (sTests.powerProfile)
      {
        case 0:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE STANDARD Passed");
          else
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE STANDARD Failed");
          break;
        case 1:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 30VIW Passed");
          else
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 30VIW Failed");
          break;
        case 2:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 27VIW Passed");
          else
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 27VIW Failed");
          break;
        case 3:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 23VIW Passed");
          else
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 23VIW Failed");
          break;/*
        case 4:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 21VIW Passed");
          else
            sprintf(sTests.testHeader.title, "SERIAL FLASH PROFILE 21VIW Failed");
          break;*/
        default: break;
      }
      break;
    case 13:
      sprintf(sTests.chanHeader[3].title, "SDCard State");
      switch (sTests.powerProfile)
      {
        case 0:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SDCARD PROFILE STANDARD Passed");
          else
            sprintf(sTests.testHeader.title, "SDCARD PROFILE STANDARD Failed");
          break;
        case 1:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 30VISR Passed");
          else
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 30VISR Failed");
          break;
        case 2:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 27VISR Passed");
          else
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 27VISR Failed");
          break;
        case 3:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 24VISR Passed");
          else
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 24VISR Failed");
          break;/*
        case 4:
          if (testPassed)
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 21VISR Passed");
          else
            sprintf(sTests.testHeader.title, "SDCARD PROFILE 21VISR Failed");
          break;*/
        default: break;
      }
      break;
    case 14:
      sprintf(sTests.chanHeader[3].title, "HIH6130 State");
      switch (sTests.powerProfile)
      {
        case 0:
          if (testPassed)
            sprintf(sTests.testHeader.title, "HIH PROFILE STANDARD Passed");
          else
            sprintf(sTests.testHeader.title, "HIH PROFILE STANDARD Failed");
          break;
        case 1:
          if (testPassed)
            sprintf(sTests.testHeader.title, "HIH PROFILE 29VIRyTW Passed");
          else
            sprintf(sTests.testHeader.title, "HIH PROFILE 29VIRyTW Failed");
          break;
        case 2:
          if (testPassed)
            sprintf(sTests.testHeader.title, "HIH PROFILE 25VIRyTW Passed");
          else
            sprintf(sTests.testHeader.title, "HIH PROFILE 25VIRyTW Failed");
          break;/*
        case 3:
          if (testPassed)
            sprintf(sTests.testHeader.title, "HIH PROFILE 23VIRyTW Passed");
          else
            sprintf(sTests.testHeader.title, "HIH PROFILE 23VIRyTW Failed");
          break;*/
        default: break;
      }
      break;
    default: break;
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test0
* DESCRIPTION Tests the enabling and disabling of the ENERGY_DOMAIN
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test00(void *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test1
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Basic sampling test based on pushbutton switches
\**************************************************************************************************/
uint16 Tests_test01(void *pArgs)
{/*
  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain

  while(1)
  {
    Time_delay(1000000);
    Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);
    Time_delay(1000000);
    Analog_setDomain(ENERGY_DOMAIN, TRUE, 3.3);
    Util_spinWait(2000000);
    while ((GPIOC->IDR & 0x00008000) && (GPIOC->IDR & 0x00004000) && (GPIOC->IDR & 0x00002000));
    Analog_testAnalog();
  }*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test2
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Testing DAC output on port 2
\**************************************************************************************************/
uint16 Tests_test02(void *pArgs)
{/*
  float outVolts;
  while(1)
  {
    for (outVolts=0.0; outVolts < 3.3; outVolts+=.001)
      DAC_setVoltage(DAC_PORT2, outVolts);
  }*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test3
* DESCRIPTION Moves the feedback voltage up and down the SPI domain to determine the min and max
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts to
\**************************************************************************************************/
uint16 Tests_test03(void *pArgs)
{/*
  uint32 i;
  // All pins connected to this domain set to inputs to prevent leakage
  HIH613X_setup(FALSE);
  EEPROM_setup(FALSE);
  SerialFlash_setup(FALSE);

  Tests_setupSPITests(EE_CHANNEL_OVERLOAD, 6000, 3.3); // 100us sample rate

  Util_spinWait(1000000);  // Adjusting the graph so as to have a distinct beginning of the test
  for (i = 3300; i > 0; i--)
  {
    Analog_setDomain(SPI_DOMAIN, TRUE, (double)i / 1000.0);
    Util_spinWait(1000);
  }
  for (i = 0; i < 3300; i++)
  {
    Analog_setDomain(SPI_DOMAIN, TRUE, (double)i / 1000.0);
    Util_spinWait(1000);
  }

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);

  Tests_teardownSPITests(TRUE);
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test4
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to EEPROM
\**************************************************************************************************/
uint16 Tests_test04(void *pArgs)
{/*
  uint8 txBuffer[128];
  uint8 rxBuffer[128];

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(1000000); // Wait 1000ms for domains to settle

  Util_fillMemory(txBuffer, sizeof(txBuffer), 0x5A);
  EEPROM_write(txBuffer, 0, sizeof(txBuffer));

  Util_fillMemory(rxBuffer, sizeof(rxBuffer), 0x00);
  EEPROM_read(0, rxBuffer, sizeof(rxBuffer));
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test5
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to Serial Flash
\**************************************************************************************************/
uint16 Tests_test05(void *pArgs)
{/*
  uint8 txBuffer[128];
  uint8 rxBuffer[128];

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(1000000); // Wait 1000ms for domains to settle

  Util_fillMemory(txBuffer, sizeof(txBuffer), 0x5A);
  SerialFlash_write(txBuffer, 0, sizeof(txBuffer));

  Util_fillMemory(rxBuffer, sizeof(rxBuffer), 0x00);
  SerialFlash_read(0, rxBuffer, sizeof(rxBuffer));
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to the SDCard
\**************************************************************************************************/
uint16 Tests_test06(void *pArgs)
{/*
  uint8 txBuffer[128];
  uint8 rxBuffer[128];

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(1000000); // Wait 1000ms for domains to settle

  SDCard_initDisk();

  Util_fillMemory(txBuffer, sizeof(txBuffer), 0x5A);
  SDCard_write(txBuffer, 0, sizeof(txBuffer));

  Util_fillMemory(rxBuffer, sizeof(rxBuffer), 0x00);
  SDCard_read(0, rxBuffer, sizeof(rxBuffer));
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test7
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test07(void *pArgs)
{ 
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test8
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test08(void *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test9
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test09(void *pArgs)
{/*
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

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  ADC_startSampleTimer(TIME_HARD_TIMER_TIMER3, 6000);     // Start timer3 triggered ADCs at 100us sample rate

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test10
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test10(void *pArgs)
{/*
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

  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(100000); // Wait 100ms for domains to settle

  ADC_openPort(ADC_PORT1, adc1Config);        // initializes the ADC, gated by timer3 overflow
  ADC_openPort(ADC_PORT2, adc2Config);
  ADC_openPort(ADC_PORT3, adc3Config);
  ADC_getSamples(ADC_PORT1, TESTS_MAX_SAMPLES); // Notify App when sample buffer is full
  ADC_getSamples(ADC_PORT2, TESTS_MAX_SAMPLES);
  ADC_getSamples(ADC_PORT3, TESTS_MAX_SAMPLES);
  sTests.adc1.isSampling = TRUE;
  sTests.adc2.isSampling = TRUE;
  sTests.adc3.isSampling = TRUE;
  ADC_startSampleTimer(TIME_HARD_TIMER_TIMER3, 300);     // Start timer3 triggered ADCs at 5us sample rate

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0xFF);
  EEPROM_write(&sTests.comms.rxBuffer[0], (uint8*)0, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0xAA);
  EEPROM_write(&sTests.comms.rxBuffer[0], (uint8*)128, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x55);
  EEPROM_write(&sTests.comms.rxBuffer[0], (uint8*)256, 128);

  Util_fillMemory(&sTests.comms.rxBuffer[0], 128, 0x00);
  EEPROM_write(&sTests.comms.rxBuffer[0], (uint8*)384, 128);

  // Complete the samples
  while(sTests.adc1.isSampling || sTests.adc2.isSampling || sTests.adc3.isSampling);
  ADC_stopSampleTimer(TIME_HARD_TIMER_TIMER3);

  // HACK: let EEPROM state be channel 22
  sTests.periphState.channel = 22;

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
  sTests.chanHeader[3].chanNum  = sTests.periphState.channel;
  sTests.chanHeader[3].bitRes   = (3.3 / 4096.0);

  // Return domains to initial state
  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(10000); // Wait 10ms for domains to settle
*/
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test11
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test11(void *pArgs)
{
  EEPROMResult result;
  Test11Args *pTestArgs = pArgs;

  sTests.powerProfile = pTestArgs->profile;
  EEPROM_setPowerProfile(pTestArgs->profile);

  Tests_setupSPITests(EE_CHANNEL_OVERLOAD, pTestArgs->commonArgs.sampleRate, EEPROM_getStateVoltage());
  Time_delay(pTestArgs->commonArgs.preTestDelayUs);
  result = EEPROM_write(pTestArgs->writeBuffer, pTestArgs->pDest, pTestArgs->writeLength);
  Time_delay(pTestArgs->commonArgs.postTestDelayUs);
  Tests_teardownSPITests((EEPROM_RESULT_OK == result));

  return (EEPROM_RESULT_OK == result);
}

/**************************************************************************************************\
* FUNCTION    Tests_test12
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test uses the same technique as test11, but performs the test iteratively and
*             aggregates the results.
\**************************************************************************************************/
uint16 Tests_test12(void *pArgs)
{
  SerialFlashResult result;
  Test12Args *pTestArgs = pArgs;

  sTests.powerProfile = pTestArgs->profile;
  SerialFlash_setPowerProfile(pTestArgs->profile);

  Tests_setupSPITests(SF_CHANNEL_OVERLOAD, pTestArgs->commonArgs.sampleRate, SerialFlash_getStateVoltage());
  Time_delay(pTestArgs->commonArgs.preTestDelayUs);
  result = SerialFlash_write(pTestArgs->writeBuffer, pTestArgs->pDest, pTestArgs->writeLength);
  Time_delay(pTestArgs->commonArgs.postTestDelayUs);
  Tests_teardownSPITests((SERIAL_FLASH_RESULT_OK == result));

  return (SERIAL_FLASH_RESULT_OK == result);
}

/**************************************************************************************************\
* FUNCTION    Tests_test13
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test uses the same technique as test12, but uses the XtremeLowPower function
\**************************************************************************************************/
uint16 Tests_test13(void *pArgs)
{
  uint8 i;
  SDWriteResult writeResult;
  Test13Args *pTestArgs = pArgs;

  sTests.powerProfile = pTestArgs->profile;
  SDCard_setPowerProfile(pTestArgs->profile);
  Analog_setDomain(SPI_DOMAIN, TRUE, SDCard_getStateVoltage());
  for (i = 0; i < 5; i++)
  {
    Time_delay(300000);
    if (SDCard_initDisk())  // disk must be initialized before we can test writes to it
      break;
  }
  
  Tests_setupSPITests(SD_CHANNEL_OVERLOAD, pTestArgs->commonArgs.sampleRate, SDCard_getStateVoltage());
  Time_delay(pTestArgs->commonArgs.preTestDelayUs);
  writeResult = SDCard_write(pTestArgs->writeBuffer, pTestArgs->pDest, pTestArgs->writeLength, pTestArgs->writeWait);
  Time_delay(pTestArgs->commonArgs.postTestDelayUs);
  Tests_teardownSPITests(SD_WRITE_RESULT_OK == writeResult);

  return (SD_WRITE_RESULT_OK == writeResult);
}

/**************************************************************************************************\
* FUNCTION    Tests_test14
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts basic operation of the SerialFlash
\**************************************************************************************************/
uint16 Tests_test14(void *pArgs)
{
  HIHStatus hihResult;
  Test14Args *pTestArgs = pArgs;

  sTests.powerProfile = pTestArgs->profile;
  HIH613X_setPowerProfile(pTestArgs->profile);

  Tests_setupSPITests(HIH_CHANNEL_OVERLOAD, pTestArgs->commonArgs.sampleRate, HIH613X_getStateVoltage());
  Time_delay(pTestArgs->commonArgs.preTestDelayUs);
  hihResult = HIH613X_readTempHumidI2CBB(pTestArgs->measure, pTestArgs->read, pTestArgs->convert);
  Time_delay(pTestArgs->commonArgs.postTestDelayUs);
  Tests_teardownSPITests(HIH_STATUS_NORMAL == hihResult);

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test15
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts power profile operation of the HIH Temp/Humid module
\**************************************************************************************************/
uint16 Tests_test15(void *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test16
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts power profile operation of the SDCard
\**************************************************************************************************/
uint16 Tests_test16(void *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test17
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts basic operation of the HIH_6130_021
\**************************************************************************************************/
uint16 Tests_test17(void *pArgs)
{/*
  uint8  i;
  HIHStatus sensorStatus;
  int16  temperature, humidity;


  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,     TRUE, 3.3);  // Set domain voltage to nominal (3.25V)
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
  Time_delay(1000000); // Wait 1000ms for domains to settle

  while ((GPIOC->IDR & 0x00008000) && (GPIOC->IDR & 0x00004000) && (GPIOC->IDR & 0x00002000))
  {
    sensorStatus = HIH613X_readTempHumidI2CBB(TRUE, TRUE, TRUE);
    temperature  = (int16)(HIH613X_getTemperature() * 100);
    humidity     = (int16)(HIH613X_getHumidity()    * 100);

    i = 0;
    if (sensorStatus != HIH_STATUS_NORMAL)
      sTests.comms.txBuffer[i++] = '*';
    sTests.comms.txBuffer[i++] = 'T';
    sTests.comms.txBuffer[i++] = ':';
    sTests.comms.txBuffer[i++] = ' ';
    sTests.comms.txBuffer[i++] = (((int16)temperature % 10000  ) / 1000  ) + 0x30;
    sTests.comms.txBuffer[i++] = (((int16)temperature % 1000   ) / 100   ) + 0x30;
    sTests.comms.txBuffer[i++] = '.';
    sTests.comms.txBuffer[i++] = (((int16)temperature % 100    ) / 10    ) + 0x30;
    sTests.comms.txBuffer[i++] = (((int16)temperature % 10     ) /  1    ) + 0x30;
    sTests.comms.txBuffer[i++] = ' ';
    sTests.comms.txBuffer[i++] = ' ';
    sTests.comms.txBuffer[i++] = ' ';

    sTests.comms.txBuffer[i++] = 'H';
    sTests.comms.txBuffer[i++] = ':';
    sTests.comms.txBuffer[i++] = ' ';
    sTests.comms.txBuffer[i++] = (((int16)humidity % 10000  ) / 1000  ) + 0x30;
    sTests.comms.txBuffer[i++] = (((int16)humidity % 1000   ) / 100   ) + 0x30;
    sTests.comms.txBuffer[i++] = '.';
    sTests.comms.txBuffer[i++] = (((int16)humidity % 100    ) / 10    ) + 0x30;
    sTests.comms.txBuffer[i++] = (((int16)humidity % 10     ) /  1    ) + 0x30;

    sTests.comms.txBuffer[i++] = '\r';
    sTests.comms.txBuffer[i++] = '\n';


    // Send the header out the USBVCP
    Tests_sendData(i);

    Time_delay(250000); // Wait 250ms for next sample
  }
*/
  return SUCCESS;
}
