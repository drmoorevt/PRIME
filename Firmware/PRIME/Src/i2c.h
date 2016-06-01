#ifndef I2C_H
#define I2C_H

#include "types.h"

typedef enum
{
  I2C_CLOCK_RATE_400000 = 0,
  I2C_CLOCK_RATE_200000 = 1,
  I2C_CLOCK_RATE_100000 = 2,
  I2C_CLOCK_RATE_050000 = 3,
  I2C_CLOCK_RATE_025000 = 4,
  I2C_CLOCK_RATE_012500 = 5,
  I2C_CLOCK_RATE_MAX
} I2CClockRate;

void I2C_init(I2C_HandleTypeDef *pI2C);
bool I2C_setup(boolean state, I2CClockRate rate);
bool I2C_read(uint8_t devAddr, uint8 *pBytes, uint16 numBytes);
bool I2C_write(uint8_t devAddr, uint8 *pBytes, uint16 numBytes);
bool I2C_memRead(uint8_t devAddr, uint16_t memAddr, uint8 *pBytes, uint16 numBytes);
bool I2C_memWrite(uint8_t devAddr, uint16_t memAddr, uint8 *pBytes, uint16 numBytes);


#endif // I2C_H
