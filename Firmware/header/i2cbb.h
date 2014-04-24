#ifndef I2CBB_H
#define I2CBB_H

#include "types.h"

void I2CBB_init(void);
boolean I2CBB_setup(boolean state);
boolean I2CBB_readData(uint8 command, uint8 *pResponse, uint8 respLength);

#endif // I2C_H
