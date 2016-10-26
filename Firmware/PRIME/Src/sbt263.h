#ifndef SBT263_H
#define SBT263_H

#include "types.h"

typedef enum
{
  SBT_STATE_IDLE         = 0,
  SBT_STATE_DATA_READY   = 1,
  SBT_STATE_SENDING_CMD  = 2,
  SBT_STATE_WAITING      = 3,
  SBT_STATE_READING      = 4,
  SBT_STATE_MAX          = 5
} SBTState;

typedef enum
{
  SBT_STATUS_NORMAL  = 0x00,
  SBT_STATUS_STALE   = 0x01,
  SBT_STATUS_COMMAND = 0x02,
  SBT_STATUS_DIAG    = 0x03
} SBTStatus;

typedef enum
{
  SBT_PROFILE_STANDARD = 0,
  SBT_PROFILE_29VIRyTW = 1,
  SBT_PROFILE_25VIRyTW = 2,
  SBT_PROFILE_23VIRyTW = 3,
  SBT_PROFILE_MAX      = 4
} SBTPowerProfile;

void SBT263_init(UART_HandleTypeDef *pUART);
bool SBT263_test(void);
boolean SBT263_setup(boolean state);
SBTState SBT263_getState(void);
uint32 SBT263_getStateAsWord(void);
void SBT263_setState(SBTState state);
double SBT263_getStateVoltage(void);
void SBT263_notifyVoltageChange(double newVoltage);
boolean SBT263_setPowerProfile(SBTPowerProfile profile);
boolean SBT263_setPowerState(SBTState state, double vDomain);

#endif
