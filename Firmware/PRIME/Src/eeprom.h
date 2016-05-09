#ifndef EEPROM_H
#define EEPROM_H

typedef struct
{
  uint8 testPage[128];
} EEpromMap;

#define gpEE    ((const EEpromMap*) 0)

typedef enum
{
  EEPROM_STATE_IDLE    = 0,
  EEPROM_STATE_READING = 1,
  EEPROM_STATE_WRITING = 2,
  EEPROM_STATE_WAITING = 3,
  EEPROM_STATE_MAX     = 4
} EEPROMState;

typedef enum
{
  EEPROM_PROFILE_STANDARD = 0,
  EEPROM_PROFILE_25VIW    = 1,
  EEPROM_PROFILE_18VIW    = 2,
  EEPROM_PROFILE_14VIW    = 3,
  EEPROM_PROFILE_MAX      = 4
} EEPROMPowerProfile;

typedef enum
{
  EEPROM_RESULT_OK           = 0,
  EEPROM_RESULT_NEEDED_RETRY = 1,
  EEPROM_RESULT_ERROR        = 2
} EEPROMResult;

void EEPROM_init(void);
boolean EEPROM_setup(boolean state);
EEPROMState EEPROM_getState(void);
uint32 EEPROM_getStateAsWord(void);
double EEPROM_getStateVoltage(void);
boolean EEPROM_setPowerProfile(EEPROMPowerProfile profile);
boolean EEPROM_setPowerState(EEPROMState state, double vDomain);
void EEPROM_read(const uint8 *pSrc, uint8 *pDest, uint16 length);
EEPROMResult EEPROM_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_fill(uint8 *pDest, uint16 length, uint8 fillVal);
void EEPROM_test(void);

#endif
