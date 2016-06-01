#ifndef SI114X_H
#define SI114X_H

#include "types.h"

typedef enum
{
  SI114X_STATE_IDLE         = 0,
  SI114X_STATE_DATA_READY   = 1,
  SI114X_STATE_SENDING_CMD  = 2,
  SI114X_STATE_WAITING      = 3,
  SI114X_STATE_READING      = 4,
  SI114X_STATE_MAX          = 5
} Si114xState;

typedef enum
{
  SI114X_PROFILE_STANDARD = 0,
  SI114X_PROFILE_29VIRyTW = 1,
  SI114X_PROFILE_25VIRyTW = 2,
  SI114X_PROFILE_23VIRyTW = 3,
  SI114X_PROFILE_MAX      = 4
} Si114xPowerProfile;

void SI114X_init(void);
bool SI114X_test(void);
bool SI114X_setup(boolean state);
void SI114X_ReadAndMeasure(void *pData);
Si114xState SI114X_getState(void);
uint32 SI114X_getStateAsWord(void);
double SI114X_getStateVoltage(void);

#endif
