#ifndef EEPROM_H
#define EEPROM_H

#include "types.h"

typedef struct
{
  uint8 testPage[128];
} EEpromMap;

#define gpEE    ((const EEpromMap*) 0)

typedef enum
{
  EEPROM_STATE_IDLE       = 0,
  EEPROM_STATE_READING    = 1,
  EEPROM_STATE_WRITING    = 2,
  EEPROM_STATE_WAITING    = 3,
  EEPROM_STATE_MAX        = 4
} EEPROMState;

typedef enum
{
  EEPROM_PROFILE_STANDARD     = 0,
  EEPROM_PROFILE_LP_IDLE_WAIT = 1,
  EEPROM_PROFILE_MP_RW_LP_IW  = 2,
  EEPROM_PROFILE_LP_ALL       = 3,
  EEPROM_PROFILE_LPI_XLP_WAIT = 4,
  EEPROM_PROFILE_MAX          = 5
} EEPROMPowerProfile;

void EEPROM_init(void);
boolean EEPROM_setup(boolean state);
EEPROMState EEPROM_getState(void);
uint32 EEPROM_getStateAsWord(void);
boolean EEPROM_setPowerProfile(EEPROMPowerProfile profile);
boolean EEPROM_setPowerState(EEPROMState state, double vDomain);
void EEPROM_read(const uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_fill(uint8 *pDest, uint16 length, uint8 fillVal);
void EEPROM_test(void);

#endif
