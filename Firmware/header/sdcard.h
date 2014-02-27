#ifndef SDCARD_H
#define SDCARD_H

#include "types.h"

typedef struct
{
  uint8 testPage[128];
} SDCardMap;

#define gpSDCard ((const SDCardMap*) 0)

typedef enum
{
  SD_CARD_IDLE,
  SD_CARD_WRITING,
  SD_CARD_WAITING,
  SD_CARD_READBACK
} SDCardState;

typedef enum
{
  SD_CARD_CHIP     =     0x60,
  SD_CARD_BLOCK4   =     0x20,
  SD_CARD_BLOCK32  =     0x54,
  SD_CARD_BLOCK64  =     0xD8
} SDCardBlockSize;

void    SDCard_init(void);
void    SDCard_test(void);
boolean SDCard_setup(void);
void    SDCard_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
uint8   SDCard_eraseFlash(uint8 *pDest, SDCardBlockSize blockSize);
uint8   SDCard_protectFlash(boolean bProtect);

#endif
