#ifndef HIH613X_H
#define HIH613X_H

#include "types.h"

typedef enum
{
  HIH_IDLE         = 0,
  HIH_TRANSMITTING = 1,
  HIH_WAITING      = 2,
  HIH_READING      = 3,
  HIH_NUM_STATES   = 4,
} HIHState;

typedef enum
{
  HIH_NORMAL  = 0x00,
  HIH_STALE   = 0x01,
  HIH_COMMAND = 0x02,
  HIH_DIAG    = 0x03
} HIHStatus;

void HIH613X_init(void);
boolean HIH613X_setup(boolean state);
HIHState HIH613X_getState(void);
boolean HIH613X_setPowerState(HIHState state, double vDomain);
void HIH613X_readTempHumidSPI(void);
HIHStatus HIH613X_readTempHumidI2CBB(boolean measure, boolean read, boolean convert);

double HIH613X_getHumidity(void);
double HIH613X_getTemperature(void);

#endif
