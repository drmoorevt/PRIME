#ifndef HIH613X_H
#define HIH613X_H

#include "types.h"

typedef enum
{
  HIH_NORMAL  = 0x00,
  HIH_STALE   = 0x01,
  HIH_COMMAND = 0x02,
  HIH_DIAG    = 0x03
} HIHStatus;

void HIH613X_init(void);
void HIH613X_readTempHumidSPI(void);
HIHStatus HIH613X_readTempHumidI2CBB(boolean measure, boolean read, boolean convert);

double HIH613X_getHumidity(void);
double HIH613X_getTemperature(void);

#endif
