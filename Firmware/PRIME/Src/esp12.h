#ifndef ESP12_H
#define ESP12_H

#include "types.h"

typedef enum
{
  ESP_STATE_IDLE         = 0,
  ESP_STATE_DATA_READY   = 1,
  ESP_STATE_SENDING_CMD  = 2,
  ESP_STATE_WAITING      = 3,
  ESP_STATE_READING      = 4,
  ESP_STATE_MAX          = 5
} ESPState;

typedef enum
{
  ESP_STATUS_NORMAL  = 0x00,
  ESP_STATUS_STALE   = 0x01,
  ESP_STATUS_COMMAND = 0x02,
  ESP_STATUS_DIAG    = 0x03
} ESPStatus;

typedef enum
{
  ESP_PROFILE_STANDARD = 0,
  ESP_PROFILE_29VIRyTW = 1,
  ESP_PROFILE_25VIRyTW = 2,
  ESP_PROFILE_23VIRyTW = 3,
  ESP_PROFILE_MAX      = 4
} ESPPowerProfile;

void ESP12_init(UART_HandleTypeDef *pUART);
bool ESP12_test(void);
boolean ESP12_setup(boolean state);
ESPState ESP12_getState(void);
uint32 ESP12_getStateAsWord(void);
void ESP12_setState(ESPState state);
double ESP12_getStateVoltage(void);
void ESP12_notifyVoltageChange(double newVoltage);
boolean ESP12_setPowerProfile(ESPPowerProfile profile);
boolean ESP12_setPowerState(ESPState state, double vDomain);

#endif
