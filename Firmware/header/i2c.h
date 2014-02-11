#ifndef I2C_H
#define I2C_H

#include "types.h"

void I2C_init(void);
void I2C_read(uint8 *pBytes, uint16 numBytes);
void I2C_send7bitAddress(uint8 address, uint8 direction);
void I2C_write(const uint8 *pBytes, uint16 numBytes);

#endif // I2C_H
