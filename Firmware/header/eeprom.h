#ifndef EEPROM_H
#define EEPROM_H

#include "types.h"

typedef struct
{
  uint8_t testPage[128];
} EEpromMap;

typedef struct
{
  uint8_t testPage[128];
} FlashMap;

#define gpEE    ((const EEpromMap*) 0)
#define gpFlash ((const FlashMap*) 0)

typedef enum 
{
  SF_CHIP     =     0x60,
  SF_BLOCK4   =     0x20,
  SF_BLOCK32  =     0x54,
  SF_BLOCK64  =     0xD8
}SFBlockSize;

typedef enum
{
  EEPROM_IDLE,
  EEPROM_WRITING,
  EEPROM_WAITING,
  EEPROM_READBACK
} EEPROMState;

EEPROMState EEPROM_getState(void);
void EEPROM_init(void);
void EEPROM_readEE(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_writeEE(uint8 *pSrc, uint8 *pDest, uint16 length);
void EEPROM_zeroEE(uint8 *pDest, uint16 length);
void EEPROM_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean EEPROM_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
uint8 EEPROM_eraseFlash(uint8 *pDest, SFBlockSize blockSize);
uint8 EEPROM_protectFlash(boolean bProtect);
void EEPROM_test(void);

#endif
