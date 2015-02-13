#ifndef I2CBB_H
#define I2CBB_H

#include "types.h"

typedef enum
{
  I2CBB_CLOCK_RATE_1500000 = 0,
  I2CBB_CLOCK_RATE_800000  = 1,
  I2CBB_CLOCK_RATE_400000  = 2,
  I2CBB_CLOCK_RATE_MAX
} I2CBBClockRate;

void I2CBB_init(void);
boolean I2CBB_setup(boolean state, I2CBBClockRate rate);
boolean I2CBB_readData(uint8 command, uint8 *pResponse, uint8 respLength);

#endif // I2C_H
