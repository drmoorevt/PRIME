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
  SERIAL_FLASH_IDLE,
  SERIAL_FLASH_WRITING,
  SERIAL_FLASH_WAITING,
  SERIAL_FLASH_READBACK
} SerialFlashState;

typedef enum
{
  SERIAL_FLASH_CHIP     =     0x60,
  SERIAL_FLASH_BLOCK4   =     0x20,
  SERIAL_FLASH_BLOCK32  =     0x54,
  SERIAL_FLASH_BLOCK64  =     0xD8
} SerialFlashBlockSize;

void    SerialFlash_init(void);
void    SerialFlash_test(void);
void    SerialFlash_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SerialFlash_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length);
uint8   SerialFlash_eraseFlash(uint8 *pDest, SerialFlashBlockSize blockSize);
uint8   SerialFlash_protectFlash(boolean bProtect);

#endif
