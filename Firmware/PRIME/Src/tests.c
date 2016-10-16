#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"  // For the BSP_LED toggling
#include "analog.h"
#include "crc.h"
#include "eeprom.h"
#include "extusb.h"
#include "extmem.h"
#include "hih613x.h"
#include "powercon.h"
#include "sdcard.h"
#include "m25px.h"
#include "plr5010d.h"
#include "spi.h"
#include "tests.h"
#include "time.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILE_ID TESTS_C

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
  uint8_t   channel;
  uint32_t  numSamples;
  bool      isSampling;
  uint16_t  *pSampleBuffer;
} Samples;

typedef struct
{
  uint32       headerBytes;  // The size of this struct
  char         title[64];    // Title of this test
  uint32       timeScale;    // Time between samples in micro seconds
  uint32       bytesPerChan; // Number of bytes to expect per channel
  uint32       numChannels;  // Total number of channels
  SDCardTiming timing;       // This will ultimately be a union for devices with variable timing
} TestHeader;

typedef struct
{
  uint8  chanNum;
  char   title[31];
  double bitRes;
} ChanHeader;

typedef struct
{
  uint32_t  sampRate;
  uint32_t  testLen;        // Estimated duration of the operation
  uint32_t  opDelay[4];     // Time / Energy delays associated with the operation
  uint32_t  profile;        // The power profile that the device will use to complete the operation
  uint32_t  preTestDelay;   // Time to wait while sampling before performing the operation
  uint32_t  postTestDelay;  // Time to wait while sampling after performing the operation
  uint8_t  *pDst;           // Destination for data (buf) within the peripheral
  uint32_t  len;            // Length of data buffer to write
  uint8_t   buf[1024];      // Data buffer to be written to the peripheral
} TestArgs;

SDRAMMap *GPSDRAM = (SDRAMMap *)(SDRAM_DEVICE_ADDR + BUFFER_OFFSET);

static struct
{
  TestHeader testHeader;
  ChanHeader chanHeader[4];
  uint8  testToRun;
  TestArgs testArgs;
  TestState state;
  uint32_t numSamps;
  uint32 (*getPeriphState)(void);
  volatile Samples adc1;
  volatile Samples adc2;
  volatile Samples adc3;
  volatile Samples periphState;
  volatile struct
  {
    uint32 bytesReceived;
    uint32 bytesToReceive;
    boolean portOpen;
    boolean receiving;
    boolean rxTimeout;
    boolean transmitting;
    __attribute__((aligned)) uint8 rxBuffer[512];
    __attribute__((aligned)) uint8 txBuffer[512];
  } comms;
  uint32_t sampIdx;
} sTests;

uint16 Tests_test00(TestArgs *pArgs);
uint16 Tests_test01(TestArgs *pArgs);
uint16 Tests_test02(TestArgs *pArgs);
uint16 Tests_test03(TestArgs *pArgs);
uint16 Tests_test04(TestArgs *pArgs);
uint16 Tests_test05(TestArgs *pArgs);
uint16 Tests_test06(TestArgs *pArgs);
uint16 Tests_test07(TestArgs *pArgs);
uint16 Tests_test08(TestArgs *pArgs);
uint16 Tests_test09(TestArgs *pArgs);
uint16 Tests_test10(TestArgs *pArgs);
uint16 Tests_test11(TestArgs *pArgs);
uint16 Tests_test12(TestArgs *pArgs);
uint16 Tests_test13(TestArgs *pArgs);
uint16 Tests_test14(TestArgs *pArgs);
uint16 Tests_test15(TestArgs *pArgs);
uint16 Tests_test16(TestArgs *pArgs);
uint16 Tests_test17(TestArgs *pArgs);

void Tests_sendData(uint16 numBytes);
bool Tests_sendBinaryResults(Samples *adcBuffer);
bool Tests_sendHeaderInfo(void);

// A test function returns the number of data bytes and takes argc/argv parameters
typedef uint16 (*TestFunction)(TestArgs *pArgs);

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
  Util_fillMemory(&sTests, sizeof(sTests), 0x00);
  sTests.adc1.pSampleBuffer        = &GPSDRAM->samples[0][0];
  sTests.adc2.pSampleBuffer        = &GPSDRAM->samples[1][0];
  sTests.adc3.pSampleBuffer        = &GPSDRAM->samples[2][0];
  sTests.periphState.pSampleBuffer = &GPSDRAM->samples[3][0];
  
  // Reset all devices to default domain
  Device i;
  for (i = DEVICE_EEPROM; i < DEVICE_MAX; i++)
    PowerCon_setDeviceDomain(i, VOLTAGE_DOMAIN_0);
  PowerCon_setDeviceDomain(DEVICE_WIFIMOD, VOLTAGE_DOMAIN_NONE);
  PowerCon_setDeviceDomain(DEVICE_BTMOD, VOLTAGE_DOMAIN_NONE);
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
  sTests.comms.receiving = TRUE;
  sTests.comms.rxTimeout = FALSE;
  sTests.comms.bytesToReceive = numBytes;
  //ExtUSB_flushRxBuffer();
  sTests.comms.bytesReceived = ExtUSB_rx((uint8_t *)&sTests.comms.rxBuffer, numBytes, timeout);
}

/**************************************************************************************************\
* FUNCTION    Tests_sendData
* DESCRIPTION Sends the specified number of bytes out the USBVCP configured for tests
* PARAMETERS  numBytes: The number of bytes from the txBuffer to send out
* RETURNS     Nothing (uses callbacks via the USBVCP notifyXXX mechanism)
\**************************************************************************************************/
void Tests_sendData(uint16 numBytes)
{
  ExtUSB_tx((uint8 *)&sTests.comms.txBuffer[0], numBytes);
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
  uint32 size, testToRun = 0, argLen = 0;
  ExtUSB_flushRxBuffer();
  
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
    argLen    = sTests.comms.rxBuffer[5];
    size      = MIN_PACKET_SIZE + argLen;
    hasValidPacket = (0x0000 == CRC_crc16(0x0000, CRC16_POLY_CCITT_STD, 
                      (uint8 *)&sTests.comms.rxBuffer, size));
  } while (!hasValidPacket);

  memcpy(&sTests.testArgs, (uint8_t *)&sTests.comms.rxBuffer[6], argLen);
  return testToRun;
}

/**************************************************************************************************\
* FUNCTION    Tests_notifyConversionComplete
* DESCRIPTION This function is called upon interrupt indicating that numSamples have been taken
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Tests_notifyConversionComplete(ADCPort port, uint32_t chan, uint32 numSamples)
{
  uint16_t *pBuffer;
  volatile Samples *pSampleSet;
  switch (port)
  {
    case ADC_PORT1: pSampleSet = &sTests.adc1;        pBuffer = &GPSDRAM->samples[0][0]; break;
    case ADC_PORT2: pSampleSet = &sTests.adc2;        pBuffer = &GPSDRAM->samples[1][0]; break;
    case ADC_PORT3: pSampleSet = &sTests.adc3;        pBuffer = &GPSDRAM->samples[2][0]; break;
    default:        pSampleSet = &sTests.periphState; pBuffer = &GPSDRAM->samples[3][0]; break;
  }
  pSampleSet->channel    = chan;
  pSampleSet->isSampling = FALSE;
  pSampleSet->numSamples = numSamples;
  pSampleSet->pSampleBuffer = pBuffer;
}

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
  sTests.testArgs.len           = 50000;
  sTests.testArgs.opDelay[0]    = 45000;
  sTests.testArgs.opDelay[3]    = 600000;
  sTests.testArgs.profile       = HIH_PROFILE_STANDARD;
  sTests.testArgs.preTestDelay  = 1000;
  sTests.testArgs.testLen       = 50000;
  sTests.testArgs.sampRate      = 1;
  sTests.testArgs.postTestDelay = 1000;
  sTests.testArgs.buf[0] = 1;
  sTests.testArgs.buf[1] = 1;
  sTests.testArgs.buf[2] = 1;
  Tests_test14(&sTests.testArgs);
  double temperature = HIH613X_getTemperature();
  double humidity = HIH613X_getHumidity();
  double totalEnergy = 0;
  uint32_t i;
  for (i = 0; i < sTests.adc3.numSamples; i++)
    totalEnergy += sTests.adc3.pSampleBuffer[i];
  
  Tests_sendData(sprintf((char *)sTests.comms.txBuffer, 
    "Temperature: %f, Humidity: %f, Energy: %f\r\n", temperature, humidity, totalEnergy));
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
  /*
  double args[3] = {3.3, 100, 1};
  uint32_t numAvgs = 10;
  memcpy(&args[2], &numAvgs, sizeof(numAvgs));
  while (1)
  {
    for (args[0] = 1.8; args[0] < 3.31; args[0] = args[0] + .1)
    {
      for (args[1] = 0; args[1] < 250; args[1] = args[1] + 10)
      {
        memcpy(sTests.testArgs.buf, args, sizeof(args));
        Tests_test01(&sTests.testArgs);
      }
    }
  }
  */
  /*
  while(1)
  {
    sTests.adc1.pSampleBuffer        = &GPSDRAM->samples[0][0];  // Reset sample buffers to
    sTests.adc2.pSampleBuffer        = &GPSDRAM->samples[1][0];  // initial values so we can
    sTests.adc3.pSampleBuffer        = &GPSDRAM->samples[2][0];  // send out the data
    sTests.periphState.pSampleBuffer = &GPSDRAM->samples[3][0];  // chronologically
    Tests_runTest14();
    Time_delay(100000);
  }
  */
  /*
  while(1)
  {
    Time_delay(1000*1000);
    BSP_LED_Toggle(LED3);
  }
  */
  switch (sTests.state)
  {
    case TEST_IDLE:  // Clear test data and setup listening for commands on the comm port
      Tests_init();
      sTests.testToRun = Tests_getTestToRun();
      sTests.state = TEST_WAITING;
      break;
    case TEST_WAITING:
      sTests.state = TEST_RUNNING; // No need to wait
      break;
    case TEST_RUNNING:
      testFunctions[sTests.testToRun](&sTests.testArgs);
      sTests.adc1.pSampleBuffer        = &GPSDRAM->samples[0][0];  // Reset sample buffers to
      sTests.adc2.pSampleBuffer        = &GPSDRAM->samples[1][0];  // initial values so we can
      sTests.adc3.pSampleBuffer        = &GPSDRAM->samples[2][0];  // send out the data
      sTests.periphState.pSampleBuffer = &GPSDRAM->samples[3][0];  // chronologically
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
          Tests_receiveData(1, 1000);
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

/**************************************************************************************************\
* FUNCTION    Tests_sendHeaderInfo
* DESCRIPTION 
* PARAMETERS  
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
bool Tests_sendHeaderInfo(void)
{
  uint32_t i, txBufOffset = 0;
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
  { // Send the header + CRC out the VCP
    Tests_sendData(txBufOffset);
    Tests_receiveData(1, 1000); // Wait for the ack
    if (sTests.comms.rxBuffer[0] == 0x54) // reset attempt
      return FALSE;
    tfAck = (0x11 == sTests.comms.rxBuffer[0]);
  }
  return tfAck;
}

/**************************************************************************************************\
* FUNCTION    Tests_sendBinaryResults
* DESCRIPTION 
* PARAMETERS  
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
bool Tests_sendBinaryResults(Samples *adcBuffer)
{
  uint8 *pData = (uint8 *)adcBuffer->pSampleBuffer;
  uint16_t crc = 0;
  //uint16_t crc = CRC_crc16(0x0000, CRC16_POLY_CCITT_STD, pData, sTests.testHeader.bytesPerChan);
  
  while(1)
  {
    ExtUSB_flushRxBuffer();
    ExtUSB_tx(pData, sTests.testHeader.bytesPerChan);  // Send the data
    ExtUSB_tx((uint8_t *)&crc, sizeof(crc));           // Send the CRC
    Tests_receiveData(1, 5000);                        // Wait for the ack
    if (sTests.comms.bytesReceived > 0)
    {
      switch (sTests.comms.rxBuffer[0])
      {
        case 0x54: return false;  // Hard reset, inform caller we need to go to zero
        case 0x42: return true;   // All good in the hood, proceed
        default:   break;         // Anything else is a 'please resend'
      } 
    }
    else
    {
      return false; // No ACK or NAK. Quit.
    }
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_setupSPITests
* DESCRIPTION Common setup actions for SPI tests
* PARAMETERS  reloadVal -- the sampling rate for SPI tests
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
static void Tests_setupSPITests(Device device, TestArgs *pArgs)
{
  uint32_t numSamps = (pArgs->preTestDelay + pArgs->testLen + pArgs->postTestDelay)/pArgs->sampRate;
  
  // Match the continuously variable domain (CVD) to the MCU domain voltage:
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, Analog_getADCVoltage(ADC_DOM0_VOLTAGE, 10));
  double refVolts = Analog_getADCVoltage(ADC_DOM0_VOLTAGE, 100);
  
  // Prepare data structures for retrieval
  sTests.testHeader.timeScale    = pArgs->sampRate; // in microseconds
  sTests.testHeader.numChannels  = 4;
  sTests.testHeader.bytesPerChan = numSamps * sizeof(uint16_t);
  sTests.chanHeader[0].chanNum   = 0;
  sTests.chanHeader[0].bitRes    = (refVolts / 4096.0) * 2;  // Voltage measurements are div2
  sTests.chanHeader[1].chanNum   = 1;
  sTests.chanHeader[1].bitRes    = (((refVolts / 4096.0) / 0.1) / 100) * 1000;  // I = (V/R) / Gain
  sTests.chanHeader[2].chanNum   = 2;
  sTests.chanHeader[2].bitRes    = (((refVolts / 4096.0) / 0.1) / 100) * 1000;  // I = (V/R) / Gain
  sTests.chanHeader[3].chanNum   = sTests.periphState.channel;
  switch (sTests.testToRun)  // Normalize these resolutions based on max number of states
  {
    case 11: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / EEPROM_STATE_MAX) / 4096.0); break;
    case 12: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / M25PX_STATE_MAX)  / 4096.0); break;
    case 13: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / SDCARD_STATE_MAX) / 4096.0); break;
    case 14: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / HIH_STATE_MAX)    / 4096.0); break;
    default: sTests.chanHeader[3].bitRes   = (3.3 * (4096.0 / 5)                / 4096.0); break;
  }
  
  // Disable all interrupts (except for the adc trigger which will be enabled last)
  DISABLE_SYSTICK_INTERRUPT();
  uint32_t i;
  for (i = 0; i <= DMA2D_IRQn; i++)
    NVIC_DisableIRQ((IRQn_Type)i);

  switch (device)
  {
    case DEVICE_EEPROM:    Analog_setPeriphStatePointer(EEPROM_getStatePointer()); break;
    case DEVICE_NORFLASH:  Analog_setPeriphStatePointer(M25PX_getStatePointer()); break;
    case DEVICE_SDCARD:    Analog_setPeriphStatePointer(SDCard_getStatePointer()); break;
    case DEVICE_TEMPSENSE: Analog_setPeriphStatePointer(HIH613X_getStatePointer()); break;
    default: break;
  }
  
  Analog_configureADC(ADC_DOM2_VOLTAGE,    sTests.adc1.pSampleBuffer,        numSamps);
  Analog_configureADC(ADC_DOM2_INCURRENT,  sTests.adc2.pSampleBuffer,        numSamps);
  Analog_configureADC(ADC_DOM2_OUTCURRENT, sTests.adc3.pSampleBuffer,        numSamps);
  Analog_configureADC(ADC_PERIPH_STATE,    sTests.periphState.pSampleBuffer, numSamps);
  
  sTests.periphState.channel    = device;   // For sorting out the state of the peripheral
  sTests.periphState.numSamples = numSamps; // decremented on each sample
  
  // Begin the sampling by turning on the associated sample timer
  sTests.adc1.isSampling        = TRUE;
  sTests.adc2.isSampling        = TRUE;
  sTests.adc3.isSampling        = TRUE;
  sTests.periphState.isSampling = TRUE;
  
  // Fill output current sample buffer with 0xFF so we can energy accumulation despite DMA
  memset(&GPSDRAM->samples[SDRAM_CHANNEL_OUTCURRENT][0], 0xFF, 
         sizeof(GPSDRAM->samples[SDRAM_CHANNEL_OUTCURRENT]));
  
  // Begin sampling here!
  Analog_startSampleTimer(pArgs->sampRate);
  
  Time_delay(pArgs->preTestDelay);  // This helps to identify state transitions
}

/**************************************************************************************************\
* FUNCTION    Tests_teardownSPITests
* DESCRIPTION Common teardown actions for SPI tests
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
static void Tests_teardownSPITests(TestArgs *pArgs, boolean testPassed)
{
  Time_delay(pArgs->postTestDelay);  // This helps to identify state transitions
  
  // Wait for any ongoing tests to complete
  while(sTests.adc1.isSampling || 
        sTests.adc2.isSampling || 
        sTests.adc3.isSampling || 
        sTests.periphState.isSampling);
  Analog_stopSampleTimer();
    
  // Enable all previous interrupts
  ENABLE_SYSTICK_INTERRUPT();
  
  //uint32_t i;
  //for (i = 0; i < TESTS_MAX_SAMPLES; i++)
  //  GPSDRAM->samples[SDRAM_CHANNEL_DEVSTATE][i] >>= 8;

  sprintf(sTests.chanHeader[0].title, "Domain Voltage (V)");
  sprintf(sTests.chanHeader[1].title, "Domain Input Current (mA)");
  sprintf(sTests.chanHeader[2].title, "Domain Output Current (mA)");
  
  switch (sTests.testToRun)
  {
    case 11:
      sprintf(sTests.chanHeader[3].title, "EEPROM State");
      switch (pArgs->profile)
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
          break;
        default: break;
      }
      break;
    case 12:
      sprintf(sTests.chanHeader[3].title, "SerialFlash State");
      switch (pArgs->profile)
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
          break;
        default: break;
      }
      break;
    case 13:
      sTests.testHeader.timing = SDCard_getLastTiming();
      sprintf(sTests.chanHeader[3].title, "SDCard State");
      switch (pArgs->profile)
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
          break;
        default: break;
      }
      break;
    case 14:
      sprintf(sTests.chanHeader[3].title, "HIH6130 State");
      switch (pArgs->profile)
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
          break;
        default: break;
      }
      break;
    default: break;
  }
}

/**************************************************************************************************\
* FUNCTION    Tests_test0
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test00(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test1
* DESCRIPTION Takes as an input the current and voltage of DOM2, provides the input/output current
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Basic sampling test based on pushbutton switches
\**************************************************************************************************/
uint16 Tests_test01(TestArgs *pArgs)
{
  double testOutVoltage = *(double *)&pArgs->buf[0];       // The output voltage
  double testOutCurrent = (*(double *)&pArgs->buf[8]) / 2; // The output current (per channel)
  uint32_t numAvgs      = *(uint32_t *)&pArgs->buf[16];    // Number of DUT measurement averages
  bool result = true;
  
  // Assume all devices are on the MCU domain and set the output voltage
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, testOutVoltage);
  Time_delay(1000*100);
  result = PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, testOutCurrent);
  Time_delay(1000*100);
  double domVolts = Analog_getADCVoltage(ADC_DOM2_VOLTAGE, numAvgs);
  double inCurrent = Analog_getADCCurrent(ADC_DOM2_INCURRENT, numAvgs);
  double outCurrent = Analog_getADCCurrent(ADC_DOM2_OUTCURRENT, numAvgs);
  // Turn off the sink so we don't accidentally leave a huge draw on the system...
  PLR5010D_clearAllDevices();
  //result = PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 0.0);
  
  ExtUSB_tx((uint8_t *)&domVolts, sizeof(domVolts));
  ExtUSB_tx((uint8_t *)&inCurrent, sizeof(inCurrent));
  ExtUSB_tx((uint8_t *)&outCurrent, sizeof(outCurrent));
  
  return (result == true) ? SUCCESS : ERROR;
}

/**************************************************************************************************\
* FUNCTION    Tests_test2
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Testing DAC output on port 2
\**************************************************************************************************/
uint16 Tests_test02(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test3
* DESCRIPTION Moves the feedback voltage up and down the SPI domain to determine the min and max
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts to
\**************************************************************************************************/
uint16 Tests_test03(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test4
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to EEPROM
\**************************************************************************************************/
uint16 Tests_test04(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test5
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to Serial Flash
\**************************************************************************************************/
uint16 Tests_test05(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test6
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       Tests writing to the SDCard
\**************************************************************************************************/
uint16 Tests_test06(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test7
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test07(TestArgs *pArgs)
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
uint16 Tests_test08(TestArgs *pArgs)
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
uint16 Tests_test09(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test10
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test10(TestArgs *pArgs)
{
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test11
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       None
\**************************************************************************************************/
uint16 Tests_test11(TestArgs *pArgs)
{
  EEPROM_setPowerProfile((EEPROMPowerProfile)pArgs->profile);

  Tests_setupSPITests(DEVICE_EEPROM, pArgs);
  EEPROMResult result = EEPROM_write(pArgs->buf, pArgs->pDst, pArgs->len, pArgs->opDelay[0]);
  Tests_teardownSPITests(pArgs, (EEPROM_RESULT_OK == result));

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
uint16 Tests_test12(TestArgs *pArgs)
{
  M25PX_setPowerProfile((M25PXPowerProfile)pArgs->profile);

  Tests_setupSPITests(DEVICE_NORFLASH, pArgs);
  M25PXResult result = M25PX_write(pArgs->buf, pArgs->pDst, pArgs->len, 
                                                            pArgs->opDelay[0],  // erase delay
                                                            pArgs->opDelay[1]); // page write delay
  Tests_teardownSPITests(pArgs, (M25PX_RESULT_OK == result));

  return (M25PX_RESULT_OK == result);
}

/**************************************************************************************************\
* FUNCTION    Tests_test13
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       
\**************************************************************************************************/
uint16 Tests_test13(TestArgs *pArgs)
{
  Delay delay = {.tDelay = pArgs->opDelay[0], .eDelay = pArgs->opDelay[3]};
  
  SDCard_setPowerProfile((SDCardPowerProfile)pArgs->profile);
  uint8 i;
  for (i = 0; i < 5; i++)
  {
    if (SDCard_initDisk())  // disk must be initialized before we can test writes to it
      break;
    Time_delay(300000);
  }
  
  Tests_setupSPITests(DEVICE_SDCARD, pArgs);
  SDWriteResult writeResult = SDCard_write(pArgs->buf, pArgs->pDst, pArgs->len, &delay);
  Tests_teardownSPITests(pArgs, (SD_WRITE_RESULT_OK == writeResult));

  return (SD_WRITE_RESULT_OK == writeResult);
}

/**************************************************************************************************\
* FUNCTION    Tests_test14
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       
\**************************************************************************************************/
uint16 Tests_test14(TestArgs *pArgs)
{
  bool measure = pArgs->buf[0];
  bool readVal = pArgs->buf[1];
  bool convert = pArgs->buf[2];
  
  Delay delay;
  delay.tDelay = pArgs->opDelay[0];
  delay.eDelay = pArgs->opDelay[3];
  
  HIH613X_setPowerProfile((HIHPowerProfile)pArgs->profile);

  Tests_setupSPITests(DEVICE_TEMPSENSE, pArgs);
  HIHStatus hihResult = HIH613X_readTempHumidI2C(measure, readVal, convert, &delay);
  Tests_teardownSPITests(pArgs, (HIH_STATUS_NORMAL == hihResult));

  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    Tests_test15
* DESCRIPTION
* PARAMETERS  None
* RETURNS     Number of bytes generated by the test
* NOTES       This test attempts power profile operation of the HIH Temp/Humid module
\**************************************************************************************************/
uint16 Tests_test15(TestArgs *pArgs)
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
uint16 Tests_test16(TestArgs *pArgs)
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
uint16 Tests_test17(TestArgs *pArgs)
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
