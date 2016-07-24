#ifndef EXTUSB_H
#define EXTUSB_H

#include "types.h"

bool ExtUSB_init(void);
bool ExtUSB_testUSB(void);
void ExtUSB_flushRxBuffer(void);
bool ExtUSB_tx(uint8_t *pSrc, uint32_t len);
uint32_t ExtUSB_rx(uint8_t *pDst, const uint32_t len, uint32_t timeout);

#endif
