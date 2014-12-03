#ifndef USBVCP_H
#define USBVCP_H

#include "types.h"

typedef struct
{
  uint32 bitrate;    /* Baud Rate */
  uint8  format;     /* Stop bits */
  uint8  paritytype; /* Parity */
  uint8  datatype;   /* Bits per word*/
} VCPLineCoding;

typedef enum
{
  USB_PORT_VCP = 0,
  USB_PORT_MAX = 1
} USBPort;

typedef enum
{
  USBVCP_EVENT_RX_COMPLETE  = 0,
  USBVCP_EVENT_RX_TIMEOUT   = 1,
  USBVCP_EVENT_RX_INTERRUPT = 2,
  USBVCP_EVENT_TX_COMPLETE  = 3,
} USBVCPEvent;

typedef struct
{
  void (*appReceiveBuffer);
  void (*appTransmitBuffer);
  void (*appNotifyCommsEvent)(USBVCPEvent, uint32);
} USBVCPCommConfig;

void USBVCP_init(void);
boolean USBVCP_send(uint8 *pSrc, uint32 numBytes);
boolean USBVCP_openPort(USBVCPCommConfig config);
boolean USBVCP_closePort(boolean closeInterface);
boolean USBVCP_receive(uint32 numBytes, uint32 timeout, boolean interChar);

uint32  USBVCP_stopReceive(void);

#endif /* USBVCP_H */
