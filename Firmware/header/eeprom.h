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
  EEPROM_IDLE       = 0,
  EEPROM_READING    = 1,
  EEPROM_WRITING    = 2,
  EEPROM_WAITING    = 3,
  EEPROM_NUM_STATES = 4
} EEPROMState;

void EEPROM_init(void);
boolean EEPROM_setup(boolean state);
EEPROMState EEPROM_getState(void);
boolean EEPROM_setPowerState(EEPROMState state, double vDomain);
void EEPROM_readEE(const uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_fill(uint8 *pDest, uint16 length, uint8 fillVal);
void EEPROM_test(void);

#endif
