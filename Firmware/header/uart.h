/**************************************************************************************************\
 * Some terminology:
 *   UART: Hardware layer
 *   Port: The combination of peripheral and pins required for operation
 *   Comm: Software layer
\**************************************************************************************************/

#ifndef UART_H
#define UART_H

#include "types.h"

typedef enum
{
  USART_PORT1    = 0,
  USART_PORT2    = 1,
  USART_PORT3    = 2,
  UART_PORT4     = 3,
  UART_PORT5     = 4,
  USART_PORT6    = 5,
  UART_PORT_MAX  = 6
} UARTPort;

typedef enum
{
  UART_BAUDRATE_2400,
  UART_BAUDRATE_4800,
  UART_BAUDRATE_9600,
  UART_BAUDRATE_19200,
  UART_BAUDRATE_38400,
  UART_BAUDRATE_57600,
  UART_BAUDRATE_115200,
  UART_BAUDRATE_230400,
  UART_BAUDRATE_460800,
  UART_BAUDRATE_921600
} BaudRate;

typedef enum
{
  UART_FLOWCONTROL_NONE,
  UART_FLOWCONTROL_RTS,
  UART_FLOWCONTROL_CTS,
  UART_FLOWCONTROL_RTSCTS
} FlowControl;

typedef struct
{
  BaudRate    baud;
  FlowControl flow;
  boolean     rxEnabled;
  boolean     txEnabled;
} UARTConfig;

typedef enum
{
  COMMS_EVENT_RX_COMPLETE  = 0,
  COMMS_EVENT_RX_TIMEOUT   = 1,
  COMMS_EVENT_RX_INTERRUPT = 2,
  COMMS_EVENT_TX_COMPLETE  = 3,
} CommsEvent;

typedef struct
{
  UARTConfig UARTConfig;
  void (*appReceiveBuffer);
  void (*appTransmitBuffer);
  void (*appNotifyCommsEvent)(CommsEvent, uint32);
} AppCommConfig;

void UART_init(void);
void UART_stopReceive(UARTPort port);
boolean UART_openPort(UARTPort port, AppCommConfig config);
boolean UART_closePort(UARTPort port);
boolean UART_sendData(UARTPort port, uint16 numBytes);
boolean UART_receiveData(UARTPort port, uint32 numBytes, uint32 timeout);

#endif // UART_H
