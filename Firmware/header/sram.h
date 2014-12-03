#ifndef SRAM_H
#define SRAM_H

#include "types.h"

typedef enum
{
  SRAM_STATE_DISABLED  = 0,
  SRAM_STATE_RETENTION = 1,
  SRAM_STATE_ENABLED   = 2,
  SRAM_STATE_MAX       = 3
} SRAMState;

void SRAM_init(void);
boolean SRAM_setup(SRAMState state);

void SRAM_test(void);
#endif // SRAM_H
