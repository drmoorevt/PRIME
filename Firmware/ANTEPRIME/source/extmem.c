#include "extmem.h"

// Tests SDRAM by writing one byte at a time to all memory locations and reading back afterwards
boolean ExtMem_testSDRAM(void)
{
  uint32 i;
  for (i = 0; i < SDRAM_DEVICE_SIZE; i++)
  {
    *(uint8 *)(SDRAM_DEVICE_ADDR + i) = i;
  }
  for (i = 0; i < SDRAM_DEVICE_SIZE; i++)
  {
    if (*(uint8 *)(SDRAM_DEVICE_ADDR + i) != (i & 0x000000FF))
      return FALSE;
  }
  return TRUE;
}
