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
  UART_PORT1     = 0,
  UART_PORT2     = 1,
  UART_PORT3     = 2,
  UART_PORT4     = 3,
  UART_PORT5     = 4,
  UART_PORT6     = 5,
  UART_NUM_PORTS = 6
} UARTPort;

typedef enum
{
  UART_BAUDRATE_2400,
  UART_BAUDRATE_4800,
  UART_BAUDRATE_9600,
  UART_BAUDRATE_19200,
  UART_BAUDRATE_38400,
  UART_BAUDRATE_57600,
  UART_BAUDRATE_115200
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

typedef struct
{
  UARTConfig UARTConfig;
  void *appReceiveBuffer;
  void (*appNotifyReceiveComplete)(uint32);
  void *appTransmitBuffer;
  void (*appNotifyTransmitComplete)(uint32);
  void (*appNotifyUnexpectedReceive)(uint8);
} AppCommConfig;

void UART_init(void);
boolean UART_openPort(UARTPort port, AppCommConfig config);
boolean UART_closePort(UARTPort port);
boolean UART_sendData(UARTPort port, uint16 numBytes);
boolean UART_receiveData(UARTPort port, uint16 numBytes);

#endif // UART_H
