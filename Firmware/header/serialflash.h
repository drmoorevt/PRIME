#ifndef SERIALFLASH_H
#define SERIALFLASH_H

#include "types.h"

typedef struct
{
  uint8 testPage[128];
} FlashMap;

#define gpFlash ((const FlashMap*) 0)

typedef enum
{
  SF_CHIP     =     0x60,
  SF_BLOCK4   =     0x20,
  SF_BLOCK32  =     0x54,
  SF_BLOCK64  =     0xD8
} SFBlockSize;

void    SerialFlash_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SerialFlash_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
uint8   SerialFlash_eraseFlash(uint8 *pDest, SFBlockSize blockSize);
uint8   SerialFlash_protectFlash(boolean bProtect);

#endif
