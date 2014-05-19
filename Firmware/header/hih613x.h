#ifndef HIH613X_H
#define HIH613X_H

#include "types.h"

typedef enum
{
  HIH_STATE_IDLE         = 0,
  HIH_STATE_DATA_READY   = 1,
  HIH_STATE_SENDING_CMD  = 2,
  HIH_STATE_WAITING      = 3,
  HIH_STATE_READING      = 4,
  HIH_STATE_MAX          = 5
} HIHState;

typedef enum
{
  HIH_STATUS_NORMAL  = 0x00,
  HIH_STATUS_STALE   = 0x01,
  HIH_STATUS_COMMAND = 0x02,
  HIH_STATUS_DIAG    = 0x03
} HIHStatus;

void HIH613X_init(void);
boolean HIH613X_setup(boolean state);
HIHState HIH613X_getState(void);
void HIH613X_readTempHumidSPI(void);
void HIH613X_notifyVoltageChange(double newVoltage);
boolean HIH613X_setPowerState(HIHState state, double vDomain);
HIHStatus HIH613X_readTempHumidI2CBB(boolean measure, boolean read, boolean convert);

double HIH613X_getHumidity(void);
double HIH613X_getTemperature(void);

#endif
