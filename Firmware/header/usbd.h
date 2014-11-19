#ifndef USBD_H
#define USBD_H

#include "stm32f2xx.h" // Need some of these
#include "types.h"

typedef enum
{
  USBD_VCP_BAUD_RATE_1152000
} USBDBaudRate;

void USBD_init(void);

#endif
