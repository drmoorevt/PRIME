#ifndef SPI_H
#define SPI_H

#include "types.h"

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
} SPIClockRate;

void SPI_init(SPI_HandleTypeDef *pSPI);
boolean SPI_setup(boolean state, SPIClockRate rate);
void SPI_write(const uint8 *pBytes, uint32 numBytes);
void SPI_read(uint8 *pBytes, uint32 numBytes);

#endif // SPI_H
