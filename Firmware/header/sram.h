#ifndef SRAM_H
#define SRAM_H

#include "types.h"

#define SRAM_START_ADDR (0x60000000)
#define SRAM_SIZE (0x100000)  // 1MB SRAM
#define SRAM_ADDR (SRAM_SIZE / 2) // 512K x 16b

#define SRAM_NUM_CHANNELS           (4)
#define SRAM_NUM_CHANNEL_SAMPLES    (47662) // Samples per 16bit channel
#define SRAM_NUM_CHANNEL_SUMMATIONS (47662) // Accumulations per 32bit channel


typedef union
{
  struct
  {
    uint16 samples[SRAM_NUM_CHANNELS][SRAM_NUM_CHANNEL_SAMPLES];
    uint32 summations[SRAM_NUM_CHANNELS][SRAM_NUM_CHANNEL_SUMMATIONS];
  } adc;
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
