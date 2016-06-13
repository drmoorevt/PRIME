#ifndef SPI_H
#define SPI_H

#include "types.h"
#include "stm32f4xx_hal.h"

typedef enum
{
  SPI_CLOCK_RATE_90000000 = 0,
  SPI_CLOCK_RATE_45000000 = 1,
  SPI_CLOCK_RATE_22500000 = 2,
  SPI_CLOCK_RATE_11250000 = 3,
  SPI_CLOCK_RATE_05625000 = 4,
  SPI_CLOCK_RATE_02812500 = 5,
  SPI_CLOCK_RATE_01406250 = 6,
  SPI_CLOCK_RATE_00703125 = 7,
  SPI_CLOCK_RATE_MAX
} SPIRate;

typedef enum
{
  SPI_CLOCK_PHASE_1EDGE = 0,
  SPI_CLOCK_PHASE_2EDGE = 1
} SPIPhase;

typedef enum
{
  SPI_CLOCK_POLARITY_LOW  = 0x0,
  SPI_CLOCK_POLARITY_HIGH = 0x1
} SPIPolarity;

typedef enum
{
  SPI_MODE_NORMAL = 0x0,
  SPI_MODE_TI     = 0x1,
} SPITIMode;

void SPI_init(SPI_HandleTypeDef *pSPI);
boolean SPI_setup(boolean state, SPIRate rate, uint32_t phase, uint32_t pol, SPITIMode mode);
void SPI_write(const uint8 *pBytes, uint32 numBytes);
void SPI_read(uint8 *pBytes, uint32 numBytes);

#endif // SPI_H
