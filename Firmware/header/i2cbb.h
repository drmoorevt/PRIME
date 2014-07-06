#ifndef I2CBB_H
#define I2CBB_H

#include "types.h"

typedef enum
{
  I2CBB_CLOCK_RATE_15000000 = 0,
  I2CBB_CLOCK_RATE_7500000  = 1,
  I2CBB_CLOCK_RATE_3250000  = 2,
  I2CBB_CLOCK_RATE_1625000  = 3,
  I2CBB_CLOCK_RATE_812500   = 4,
  I2CBB_CLOCK_RATE_406250   = 5,
  I2CBB_CLOCK_RATE_203125   = 6,
  I2CBB_CLOCK_RATE_101562   = 7,
  I2CBB_CLOCK_RATE_MAX
} I2CBBClockRate;

void I2CBB_init(void);
boolean I2CBB_setup(boolean state, I2CBBClockRate rate);
boolean I2CBB_readData(uint8 command, uint8 *pResponse, uint8 respLength);

#endif // I2C_H
