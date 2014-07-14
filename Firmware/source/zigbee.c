#include "stm32f2xx.h"
#include "analog.h"
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
#include "zigbee.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILE_ID ZIGBEE_C

#define ZIGBEE_PIN_RESET         (GPIO_Pin_6) // Port d

#define ZIGBEE_SELECT_RESET()   do { GPIOD->BSRRH |= 0x00000040; } while (0)
#define ZIGBEE_DESELECT_RESET() do { GPIOD->BSRRL |= 0x00000040; } while (0)

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
} sZigBee;

/**************************************************************************************************\
* FUNCTION    ZigBee_notifyCommsEvent
* DESCRIPTION Callback from the comms module indicating an event has occurred (tx,rx,to)
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ZigBee_notifyCommsEvent(CommsEvent event, uint32 arg)
{
  switch (event)
  {
    case COMMS_EVENT_RX_COMPLETE:
      sZigBee.comms.receiving = FALSE;
      break;
    case COMMS_EVENT_RX_TIMEOUT:
      sZigBee.comms.receiving = FALSE;
      sZigBee.comms.rxTimeout = TRUE;
      break;
    case COMMS_EVENT_RX_INTERRUPT:
      break;
    case COMMS_EVENT_TX_COMPLETE:
      sZigBee.comms.transmitting = FALSE;
      break;
    default:
      break;
  }
}

/**************************************************************************************************\
* FUNCTION    ZigBee_receiveData
* DESCRIPTION Will wait for either the specified number of bytes or for the timeout to expire
* PARAMETERS  numBytes: The number of bytes to wait for
*             timeout: The maximum amount time in milliseconds to wait for the data
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
* NOTES       If timeout is 0 then the function can block forever
\**************************************************************************************************/
void ZigBee_receiveData(uint32 numBytes, uint32 timeout)
{
  sZigBee.comms.receiving = TRUE;
  sZigBee.comms.rxTimeout = FALSE;
  UART_receiveData(USART_PORT3, numBytes, timeout, FALSE);
}

/**************************************************************************************************\
* FUNCTION    ZigBee_sendData
* DESCRIPTION Sends the specified number of bytes out the UART configured for zigbee
* PARAMETERS  numBytes: The number of bytes from the txBuffer to send out
* RETURNS     Nothing (uses callbacks via the UART_notifyCommsEvent mechanism)
\**************************************************************************************************/
void ZigBee_sendData(uint16 numBytes)
{
  sZigBee.comms.transmitting = TRUE;
  UART_sendData(USART_PORT3, sZigBee.comms.txBuffer, numBytes);
}

static void ZigBee_setup(boolean state);

/**************************************************************************************************\
* FUNCTION    ZigBee_init
* DESCRIPTION Initializes the ZigBee module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ZigBee_init(void)
{
  AppCommConfig comm3 = { {UART_BAUDRATE_115200, UART_FLOWCONTROL_NONE, TRUE, TRUE},
                           &sZigBee.comms.rxBuffer[0],
                           &sZigBee.comms.txBuffer[0],
                           &ZigBee_notifyCommsEvent };
  Util_fillMemory(&sZigBee, sizeof(sZigBee), 0x00);
  UART_openPort(USART_PORT3, comm3);
  sZigBee.comms.portOpen = TRUE;
  ZigBee_setup(FALSE);
}

/**************************************************************************************************\
* FUNCTION    ZigBee_setup
* DESCRIPTION Configures the ZigBee module for use
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ZigBee_setup(boolean state)
{
  // Initialize the ZigBee chip reset line
  GPIO_InitTypeDef zbCtrlPortD = {(ZIGBEE_PIN_RESET), GPIO_Mode_OUT,
                                   GPIO_Speed_25MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL,
                                   GPIO_AF_SYSTEM };
  zbCtrlPortD.GPIO_Mode = (state == TRUE) ? GPIO_Mode_OUT : GPIO_Mode_IN;
  GPIO_configurePins(GPIOD, &zbCtrlPortD);
  GPIO_setPortClock(GPIOD, TRUE);    
  ZIGBEE_SELECT_RESET();
  Time_delay(1000);
  ZIGBEE_DESELECT_RESET();
}

/**************************************************************************************************\
* FUNCTION    ZigBee_test
* DESCRIPTION Connection testing on the XBEE module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void ZigBee_test(void)
{
  uint32 i, j;
  for (i = 0; i < 0xFFFFFFFF; i++)
  {
    sprintf((char *)sZigBee.comms.txBuffer, "%u: Hello World!\r\n", i);
    Time_delay(100000);
    for (j = 0; j < sizeof(sZigBee.comms.txBuffer); j++)
      if (0x00 == sZigBee.comms.txBuffer[j])
        break;
    ZigBee_sendData(j);
  }
}
