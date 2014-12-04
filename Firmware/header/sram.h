#ifndef SRAM_H
#define SRAM_H

#include "types.h"

#define SRAM_START_ADDR (0x60000000)
#define SRAM_SIZE (0x100000)  // 1MB SRAM
#define SRAM_ADDR (SRAM_SIZE / 2) // 512K x 16b

typedef union
{
  uint16 extmem[SRAM_ADDR];
} ExtMemoryMap;

#define GPSRAM ((ExtMemoryMap *) SRAM_START_ADDR)

typedef enum
{
  SRAM_STATE_DISABLED  = 0,
  SRAM_STATE_RETENTION = 1,
  SRAM_STATE_ENABLED   = 2,
  SRAM_STATE_MAX       = 3
} SRAMState;

void SRAM_init(void);
boolean SRAM_setup(SRAMState state);

boolean SRAM_test(void);
#endif // SRAM_H
