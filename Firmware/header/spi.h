#ifndef SPI_H
#define SPI_H

#include "types.h"

void SPI_init(void);
void SPI_write(const uint8 *pBytes, uint32 numBytes);
void SPI_read(uint8 *pBytes, uint32 numBytes);

#endif // SPI_H
