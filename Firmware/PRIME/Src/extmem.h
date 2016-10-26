#ifndef EXTMEM_H
#define EXTMEM_H

#include "stm32f4xx_hal.h"
#include "types.h"

#define SDRAM_DEVICE_ADDR         ((uint32_t)0xD0000000)
#define SDRAM_DEVICE_SIZE         ((uint32_t)0x800000)  /* SDRAM device size in MBytes */
#define BUFFER_OFFSET             ((uint32_t)0x50000)   /* Offset beyond the LCD frame buffer */

bool ExtMem_testSDRAM(void);
void ExtMem_SDRAM_Initialization_sequence(uint32_t RefreshCount, SDRAM_HandleTypeDef *SdramHandle);

#endif
