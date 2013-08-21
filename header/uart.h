#ifndef UART_H
#define UART_H

#include "types.h"

#define SERIAL_BUFFER_SIZE (256)

typedef enum
{
  SERIAL_PORT1,
  SERIAL_PORT2,
  SERIAL_PORT3,
  SERIAL_PORT4,
  SERIAL_PORT5,
  SERIAL_PORT6
} SerialPort;

typedef struct
{
  struct
  {
    boolean rxComplete;
    uint16  numRx;
    uint16  rxBuffer[SERIAL_BUFFER_SIZE];
  } rx;
  struct
  {
    uint16  numTx;
    uint16  bufIdx;
    uint8   txBuffer[SERIAL_BUFFER_SIZE];
  } tx;
} FullDuplexPort;

boolean UART_init(SerialPort port);
boolean UART_isTxBufIdle(SerialPort port);
FullDuplexPort* UART_getPortPtr(SerialPort port);
boolean UART_putChar(SerialPort port, uint8 txByte);
boolean UART_sendData(SerialPort port, uint16 numBytes);
boolean UART_checkChar(SerialPort port);
uint8   UART_getChar(SerialPort port);

#endif // UART_H
