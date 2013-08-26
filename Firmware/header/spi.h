#ifndef SPI_H
#define SPI_H

#include "types.h"

void SPI_init(void);
void SPI_write(const uint8_t *pBytes, uint16_t numBytes);
void SPI_read(uint8_t *pBytes, uint16_t numBytes);

#endif // SPI_H
