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
  EEPROM_IDLE,
  EEPROM_WRITING,
  EEPROM_WAITING,
  EEPROM_READBACK
} EEPROMState;

void EEPROM_init(void);
void EEPROM_test(void);
EEPROMState EEPROM_getState(void);
void EEPROM_readEE(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_writeEE(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_writeEELowPower(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_writeEEXLP(uint8 *pSrc, uint8 *pDest, uint16 length);
void EEPROM_zeroEE(uint8 *pDest, uint16 length);

#endif
