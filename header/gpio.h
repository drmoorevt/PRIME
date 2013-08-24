#ifndef GPIO_H
#define GPIO_H

#include "types.h"

// Alternate functions are split into two registers: Low for pins 0-7, High for 8-15
typedef enum
{
  ALT_FUNC_LOW  = 0,
  ALT_FUNC_HIGH = 1
} AFOffset;

typedef struct
{
  GPIO_TypeDef *pGPIOReg;
  AFOffset altFuncOffset;
  uint32   gpioClockEnableBit;
  uint32   modeClearMask;
  uint32   modeSetMask;
  uint32   outTypeClearMask;
  uint32   outTypeSetMask;
  uint32   outSpeedClearMask;
  uint32   outSpeedSetMask;
  uint32   pullUpDownClearMask;
  uint32   pullUpDownSetMask;
  uint32   altFuncClearMask;
  uint32   altFuncSetMask;
} PinConfig;

void GPIO_init(void);

#endif // GPIO_H
