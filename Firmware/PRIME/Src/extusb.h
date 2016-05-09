#ifndef EXTUSB_H
#define EXTUSB_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

bool ExtUSB_init(void);
bool ExtUSB_tx(uint8_t *pSrc, uint32_t len);
bool ExtUSB_rx(uint8_t *pDst, uint32_t len);

#endif
