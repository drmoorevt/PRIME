#ifndef USBVCP_H
#define USBVCP_H

#include "types.h"

void USBVCP_init(void);
boolean USBVCP_send(uint8 *pSrc, uint32 numBytes);
uint32  USBVCP_stopReceive(void);
boolean USBVCP_openPort(void);
boolean USBVCP_closePort(void);
boolean USBVCP_receive(uint32 numBytes, uint32 timeout, boolean interChar);

#endif /* USBVCP_H */
