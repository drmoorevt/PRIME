#include "stm32f2xx.h"
#include "analog.h"
#include "bluetooth.h"
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

#define FILE_ID BLUETOOTH_C

#define BLUETOOTH_PIN_FACTORY_RESET (GPIO_Pin_X)
#define BLUETOOTH_PIN_RESET         (GPIO_Pin_1) // Port B
#define BLUETOOTH_PIN_CTS           (GPIO_Pin_X)
#define BLUETOOTH_PIN_RTS           (GPIO_Pin_X)

#define BLUETOOTH_SELECT_RESET()   do { GPIOB->BSRRH |= 0x00000002; } while (0)
#define BLUETOOTH_DESELECT_RESET() do { GPIOB->BSRRL |= 0x00000002; } while (0)

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
} sBluetooth;

/**************************************************************************************************\
* FUNCTION    Bluetooth_notifyCommsEvent
* DESCRIPTION Callback from the comms module indicating an event has occurred (tx,rx,to)
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Bluetooth_notifyCommsEvent(CommsEvent event, uint32 arg)
{
  switch (event)
  {
    case COMMS_EVENT_RX_COMPLETE:
      sBluetooth.comms.receiving = FALSE;
      break;
    case COMMS_EVENT_RX_TIMEOUT:
      sBluetooth.comms.receiving = FALSE;
      sBluetooth.comms.rxTimeout = TRUE;
      break;
    case COMMS_EVENT_RX_INTERRUPT:
      break;
    case COMMS_EVENT_TX_COMPLETE:
      sBluetooth.comms.transmitting = FALSE;
      break;
    default:
      break;
  }
}

/**************************************************************************************************\
* FUNCTION    Bluetooth_receiveData
* DESCRIPTION Will wait for either the specified number of bytes or for the timeout to expire
* PARAMETERS  numBytes: The number of bytes to wait for
*             timeout: The maximum amount time in milliseconds to wait for the data
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
* NOTES       If timeout is 0 then the function can block forever
\**************************************************************************************************/
void Bluetooth_receiveData(uint32 numBytes, uint32 timeout)
{
  sBluetooth.comms.receiving = TRUE;
  sBluetooth.comms.rxTimeout = FALSE;
  UART_receiveData(UART_PORT4, numBytes, timeout);
}

/**************************************************************************************************\
* FUNCTION    Bluetooth_sendData
* DESCRIPTION Sends the specified number of bytes out the UART configured for bluetooth
* PARAMETERS  numBytes: The number of bytes from the txBuffer to send out
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
\**************************************************************************************************/
void Bluetooth_sendData(uint16 numBytes)
{
  sBluetooth.comms.transmitting = TRUE;
  UART_sendData(UART_PORT4, numBytes);
}

static void Bluetooth_setup(boolean state);

/**************************************************************************************************\
* FUNCTION    Bluetooth_init
* DESCRIPTION Initializes the Bluetooth module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Bluetooth_init(void)
{
  AppCommConfig comm4 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sBluetooth.comms.rxBuffer[0],
                           &sBluetooth.comms.txBuffer[0],
                           &Bluetooth_notifyCommsEvent };
  Util_fillMemory(&sBluetooth, sizeof(sBluetooth), 0x00);
  UART_openPort(UART_PORT4, comm4);
  sBluetooth.comms.portOpen = TRUE;
  Bluetooth_setup(FALSE);
}

/**************************************************************************************************\
* FUNCTION    Bluetooth_setup
* DESCRIPTION Configures the Bluetooth module for use
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Bluetooth_setup(boolean state)
{
  // Initialize the Bluetooth chip reset line
  GPIO_InitTypeDef btCtrlPortB = {(BLUETOOTH_PIN_RESET), GPIO_Mode_OUT,
                                   GPIO_Speed_25MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL,
                                   GPIO_AF_SYSTEM };
  btCtrlPortB.GPIO_Mode = (state == TRUE) ? GPIO_Mode_OUT : GPIO_Mode_IN;
  GPIO_configurePins(GPIOB, &btCtrlPortB);
  GPIO_setPortClock(GPIOB, TRUE);    
  BLUETOOTH_SELECT_RESET();
  Time_delay(1000);
  BLUETOOTH_DESELECT_RESET();
}

/**************************************************************************************************\
* FUNCTION    Bluetooth_test
* DESCRIPTION Connection testing on the RN42XV module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void Bluetooth_test(void)
{
  uint32 i, j;
  for (i = 0; i < 0xFFFFFFFF; i++)
  {
    sprintf((char *)sBluetooth.comms.txBuffer, "%u: Hello World!\r\n", i);
    Time_delay(100000);
    for (j = 0; j < sizeof(sBluetooth.comms.txBuffer); j++)
      if (0x00 == sBluetooth.comms.txBuffer[j])
        break;
    Bluetooth_sendData(j);
  }
}
