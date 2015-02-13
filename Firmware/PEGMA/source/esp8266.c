#include "stm32f2xx.h"
#include "analog.h"
#include "ESP8266.h"
#include "crc.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "hih613x.h"
#include "sdcard.h"
#include "serialflash.h"
#include "spi.h"
#include "time.h"
#include "uart.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILE_ID ESP8266_C

#define ESP8266_PIN_RESET         (GPIO_Pin_6)

#define ESP8266_SELECT_RESET()   do { GPIOD->BSRRH |= 0x00000040; } while (0)
#define ESP8266_DESELECT_RESET() do { GPIOD->BSRRL |= 0x00000040; } while (0)

static struct
{
  struct
  {
    boolean portOpen;
    boolean receiving;
    boolean rxTimeout;
    boolean transmitting;
    __attribute__((aligned)) uint8 rxBuffer[256];
    __attribute__((aligned)) uint8 txBuffer[256];
  } comms;
} sESP8266;

static void ESP8266_setup(BaudRate baud, boolean hardReset);

/**************************************************************************************************\
* FUNCTION    ESP8266_notifyCommsEvent
* DESCRIPTION Callback from the comms module indicating an event has occurred (tx,rx,to)
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ESP8266_notifyCommsEvent(CommsEvent event, uint32 arg)
{
  switch (event)
  {
    case COMMS_EVENT_RX_COMPLETE:
      sESP8266.comms.receiving = FALSE;
      break;
    case COMMS_EVENT_RX_TIMEOUT:
      sESP8266.comms.receiving = FALSE;
      sESP8266.comms.rxTimeout = TRUE;
      break;
    case COMMS_EVENT_RX_INTERRUPT:
      break;
    case COMMS_EVENT_TX_COMPLETE:
      sESP8266.comms.transmitting = FALSE;
      break;
    default:
      break;
  }
}

/**************************************************************************************************\
* FUNCTION    ESP8266_receiveData
* DESCRIPTION Will wait for either the specified number of bytes or for the timeout to expire
* PARAMETERS  numBytes: The number of bytes to wait for
*             timeout: The maximum amount time in milliseconds to wait for the data
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
* NOTES       If timeout is 0 then the function can block forever
\**************************************************************************************************/
void ESP8266_receiveData(uint32 numBytes, uint32 timeout)
{
  sESP8266.comms.receiving = TRUE;
  sESP8266.comms.rxTimeout = FALSE;
  UART_receiveData(USART_PORT3, numBytes, timeout, FALSE);
  while(sESP8266.comms.receiving);
}

/**************************************************************************************************\
* FUNCTION    ESP8266_sendData
* DESCRIPTION Sends the specified number of bytes out the UART configured for ESP8266
* PARAMETERS  numBytes: The number of bytes from the txBuffer to send out
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
\**************************************************************************************************/
void ESP8266_sendData(uint16 numBytes)
{
  sESP8266.comms.transmitting = TRUE;
  UART_sendData(USART_PORT3, sESP8266.comms.txBuffer, numBytes);
  while(sESP8266.comms.transmitting);
}

/**************************************************************************************************\
* FUNCTION    ESP8266_init
* DESCRIPTION Initializes the ESP8266 module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ESP8266_init(void)
{
  Util_fillMemory(&sESP8266, sizeof(sESP8266), 0x00);
  ESP8266_setup(UART_BAUDRATE_9600, FALSE);
}

/**************************************************************************************************\
* FUNCTION    ESP8266_setup
* DESCRIPTION Configures the ESP8266 module for use
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ESP8266_setup(BaudRate baud, boolean hardReset)
{
  GPIO_InitTypeDef btCtrlPortD = {(ESP8266_PIN_RESET), GPIO_Mode_OUT,
                                   GPIO_Speed_25MHz, GPIO_OType_OD, GPIO_PuPd_UP,
                                   GPIO_AF_SYSTEM };
  
  AppCommConfig comm3 = { {UART_BAUDRATE_9600, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sESP8266.comms.rxBuffer[0],
                           &sESP8266.comms.txBuffer[0],
                           &ESP8266_notifyCommsEvent };
  // Open the comm port
  comm3.UARTConfig.baud = baud;
  UART_openPort(USART_PORT3, comm3);
  sESP8266.comms.portOpen = TRUE;
  
  // Initialize the ESP8266 chip reset line
  GPIO_configurePins(GPIOD, &btCtrlPortD);
  GPIO_setPortClock(GPIOD, TRUE);
  if (hardReset)
  {
    ESP8266_SELECT_RESET();
    Time_delay(1000*1000); // Ensure reset for 1s before releasing
    ESP8266_DESELECT_RESET();
    ESP8266_receiveData(85, 1000);
  }
  else
    ESP8266_DESELECT_RESET();
}

/**************************************************************************************************\
* FUNCTION    ESP8266_test
* DESCRIPTION Connection testing on the RN42XV module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
#define AP "\"Aegis\""
#define PW "\"EDBE83B9F5\""
#define IP "\"192.168.1.151\""
void ESP8266_test(void)
{
  uint32 i, len;
  uint64 time; // %lld for formatting
  double hum, temp;
  char request[256];

  do
  { // Perform a hard reset
    Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
    ESP8266_setup(UART_BAUDRATE_9600, TRUE);
  } while (strstr((char *)sESP8266.comms.rxBuffer, "ready") == NULL);
  
  do
  { // Change the working mode to AP+STA
    Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CWMODE=3\r\n"));
    ESP8266_receiveData(14, 1000);
  } while ((strstr((char *)sESP8266.comms.rxBuffer, "OK") == NULL) &&
           (strstr((char *)sESP8266.comms.rxBuffer, "H") == NULL));
  
  do
  { // List available access points and join local network
    Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CWLAP\r\n"));
    for (i = 0; (strstr((char *)sESP8266.comms.rxBuffer, "OK") == NULL) && i < 5; i++)
      ESP8266_receiveData(256, 2000);
  } while (strstr((char *)sESP8266.comms.rxBuffer, "OK") == NULL);
  
  do
  { // Join local access point
    Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CWJAP=%s,%s\r\n", AP, PW));
    ESP8266_receiveData(256, 500);
  } while ((strstr((char *)sESP8266.comms.rxBuffer, "OK") == NULL) &&
           (strstr((char *)sESP8266.comms.rxBuffer, "H") == NULL));
  
  do
  {
    Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CIFSR\r\n"));
    ESP8266_receiveData(256, 1000);
  } while (strstr((char *)sESP8266.comms.rxBuffer, "OK") == NULL);
  
  while (1)
  {
    HIH613X_readTempHumidI2CBB(TRUE, TRUE, TRUE);
    hum = HIH613X_getHumidity();
    temp = HIH613X_getTemperature();
    time = Time_getSystemTime();
    do
    {
      Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
      ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CIPSTART=\"TCP\",%s,80", IP));
      ESP8266_receiveData(256, 5000);
    } while (strstr((char *)sESP8266.comms.rxBuffer, "Error") != NULL);
    
    do
    {
      Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
      len = sprintf(request, "GET /index.php HTTP/1.0\r\n");
      ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CIPSEND=%u", len));
      ESP8266_receiveData(256, 1000);
    } while (strstr((char *)sESP8266.comms.rxBuffer, "O") == NULL);
    
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "GET /index.php HTTP/1.0\r\n"));
    do
    {
      Util_fillMemory(&sESP8266.comms.rxBuffer, sizeof(sESP8266.comms.rxBuffer), 0x00);
      ESP8266_receiveData(256, 5000);
    } while (strstr((char *)sESP8266.comms.rxBuffer, "\"main\":{") == NULL);
  }
  
  /*
  
  do
  {
    ESP8266_setup(TRUE);
    ESP8266_receiveData(100, 5000);
  } while (strstr((char *)sESP8266.comms.rxBuffer, rString) == NULL);
  */
  /*
  // Perform a soft reset
  do
  {
    ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+RST\r\n"));
    ESP8266_receiveData(100, 5000);
  } while (strstr((char *)sESP8266.comms.rxBuffer, rString) == NULL);
  */
  
  // List available access points and join local network
  ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CWLAP\r\n"));
  ESP8266_receiveData(256, 1000);
  
  // Check for success
  ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CWJAP?\r\n"));
  ESP8266_receiveData(256, 1000);
  
  // Setup a TCP Server, port 9999
  ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CIPSERVER=1,9999\r\n"));
  ESP8266_receiveData(256, 1000);
  
  // Check for module IP address
  ESP8266_sendData(sprintf((char *)sESP8266.comms.txBuffer, "AT+CIFSR\r\n"));
  ESP8266_receiveData(256, 1000);
}
