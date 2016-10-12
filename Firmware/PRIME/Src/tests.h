#ifndef TESTS_H
#define TESTS_H

#include "types.h"
#include "analog.h"
#include "extmem.h"

typedef enum
{
  SDRAM_CHANNEL_VOLTAGE    = 0,
  SDRAM_CHANNEL_INCURRENT  = 1,
  SDRAM_CHANNEL_OUTCURRENT = 2,
  SDRAM_CHANNEL_DEVSTATE   = 3,
  SDRAM_CHANNEL_COUNT      = 4
} SDRAMChannel;

#define SDRAM_NUM_TOTAL_SAMPLES   ((SDRAM_DEVICE_SIZE - BUFFER_OFFSET) / sizeof(uint16_t))
#define TESTS_MAX_SAMPLES         (SDRAM_NUM_TOTAL_SAMPLES / SDRAM_CHANNEL_COUNT)

typedef struct
{
  uint16_t samples[SDRAM_CHANNEL_COUNT][TESTS_MAX_SAMPLES];
} SDRAMMap;

extern SDRAMMap *GPSDRAM;

void Tests_init(void);
void Tests_run(void);
void Tests_notifyConversionComplete(ADCPort port, uint32_t chan, uint32 numSamples);

#endif // TESTS_H
