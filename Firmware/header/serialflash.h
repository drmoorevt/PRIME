#ifndef SERIALFLASH_H
#define SERIALFLASH_H

#include "types.h"

#pragma anon_unions

typedef enum
{
  SERIAL_FLASH_IDLE,
  SERIAL_FLASH_ERASING,
  SERIAL_FLASH_WRITING,
  SERIAL_FLASH_WAITING,
  SERIAL_FLASH_READBACK
} SerialFlashState;

typedef enum
{
  SF_PAGE_SIZE      =     0x000100,
  SF_SUBSECTOR_SIZE =     0x001000,
  SF_SECTOR_SIZE    =     0x010000,
  SF_CHIP_SIZE      =     0x200000,
} SerialFlashSize;

#define SF_SECTORS_PER_CHIP      (SF_CHIP_SIZE / SF_SECTOR_SIZE)
#define SF_SUBSECTORS_PER_CHIP   (SF_CHIP_SIZE / SF_SUBSECTOR_SIZE)
#define SF_PAGES_PER_CHIP        (SF_CHIP_SIZE / SF_PAGE_SIZE)
#define SF_SUBSECTORS_PER_SECTOR (SF_SECTOR_SIZE / SF_SUBSECTOR_SIZE)
#define SF_PAGES_PER_SECTOR      (SF_SECTOR_SIZE / SF_PAGE_SIZE)
#define SF_PAGES_PER_SUBSECTOR   (SF_SUBSECTOR_SIZE / SF_PAGE_SIZE)

typedef struct
{
  uint8 byte[SF_PAGE_SIZE];
} FlashPage;

typedef struct
{
  union
  {
    uint8     byte[SF_SUBSECTOR_SIZE];
    FlashPage page[SF_PAGES_PER_SUBSECTOR];
  };
} FlashSubSector;

typedef struct
{
  union
  {
    uint8          byte[SF_SECTOR_SIZE];
    FlashPage      page[SF_PAGES_PER_SECTOR];
    FlashSubSector subSector[SF_SUBSECTORS_PER_SECTOR];
  };
} FlashSector;

typedef struct
{
  union
  {
    uint8          byte[SF_CHIP_SIZE];
    FlashPage      page[SF_PAGES_PER_CHIP];
    FlashSubSector subSector[SF_SUBSECTORS_PER_CHIP];
    FlashSector    sector[SF_SECTORS_PER_CHIP];
  };
} FlashOrganization;

typedef struct
{
  FlashSubSector subSector;
} FlashMap;

#define gpSerialFlash ((const FlashMap*) 0)


typedef struct
{
  uint8  manufacturerId;
  uint8  memoryType;
  uint8  memoryCapacity;
  uint8  uidLength;
  uint8  uniqueId[16];
} FlashID;

void    SerialFlash_init(void);
void    SerialFlash_test(void);
boolean SerialFlash_read(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SerialFlash_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SerialFlash_writeLP(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SerialFlash_writeXLP(uint8 *pSrc, uint8 *pDest, uint16 length);
FlashID SerialFlash_readFlashID(void);

#endif
