#ifndef SPI_H
#define SPI_H

#include "types.h"

typedef enum
{
  SPI_CLOCK_RATE_15000000 = 0,
  SPI_CLOCK_RATE_7500000  = 1,
  SPI_CLOCK_RATE_3250000  = 2,
  SPI_CLOCK_RATE_1625000  = 3,
  SPI_CLOCK_RATE_812500   = 4,
  SPI_CLOCK_RATE_406250   = 5,
  SPI_CLOCK_RATE_203125   = 6,
  SPI_CLOCK_RATE_101562   = 7,
  SPI_CLOCK_RATE_MAX
}SPIClockRate;

void SPI_init(void);
boolean SPI_setup(boolean state, SPIClockRate rate);
void SPI_write(const uint8 *pBytes, uint32 numBytes);
void SPI_read(uint8 *pBytes, uint32 numBytes);

#endif // SPI_H
