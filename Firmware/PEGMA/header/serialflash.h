#ifndef SERIALFLASH_H
#define SERIALFLASH_H

#include "types.h"

#pragma anon_unions

typedef enum
{
  SERIAL_FLASH_STATE_IDLE    = 0,
  SERIAL_FLASH_STATE_READING = 1,
  SERIAL_FLASH_STATE_ERASING = 2,
  SERIAL_FLASH_STATE_WRITING = 3,
  SERIAL_FLASH_STATE_WAITING = 4,
  SERIAL_FLASH_STATE_MAX     = 5
} SerialFlashState;

typedef enum
{
  SERIAL_FLASH_PROFILE_STANDARD = 0,
  SERIAL_FLASH_PROFILE_30VIW    = 1,
  SERIAL_FLASH_PROFILE_27VIW    = 2,
  SERIAL_FLASH_PROFILE_23VIW    = 3,
  SERIAL_FLASH_PROFILE_21VIW    = 4,
  SERIAL_FLASH_PROFILE_MAX      = 5
} SerialFlashPowerProfile;

typedef enum
{
  SERIAL_FLASH_SIZE_PAGE      = 0x000100,
  SERIAL_FLASH_SIZE_SUBSECTOR = 0x001000,
  SERIAL_FLASH_SIZE_SECTOR    = 0x010000,
  SERIAL_FLASH_SIZE_CHIP      = 0x200000
} SerialFlashSize;

typedef enum
{
  SERIAL_FLASH_RESULT_OK           = 0,
  SERIAL_FLASH_RESULT_NEEDED_RETRY = 1,
  SERIAL_FLASH_RESULT_ERROR        = 2
} SerialFlashResult;

#define SF_SECTORS_PER_CHIP      (SERIAL_FLASH_SIZE_CHIP      / SERIAL_FLASH_SIZE_SECTOR)
#define SF_SUBSECTORS_PER_CHIP   (SERIAL_FLASH_SIZE_CHIP      / SERIAL_FLASH_SIZE_SUBSECTOR)
#define SF_PAGES_PER_CHIP        (SERIAL_FLASH_SIZE_CHIP      / SERIAL_FLASH_SIZE_PAGE)
#define SF_SUBSECTORS_PER_SECTOR (SERIAL_FLASH_SIZE_SECTOR    / SERIAL_FLASH_SIZE_SUBSECTOR)
#define SF_PAGES_PER_SECTOR      (SERIAL_FLASH_SIZE_SECTOR    / SERIAL_FLASH_SIZE_PAGE)
#define SF_PAGES_PER_SUBSECTOR   (SERIAL_FLASH_SIZE_SUBSECTOR / SERIAL_FLASH_SIZE_PAGE)

typedef struct
{
  uint8 byte[SERIAL_FLASH_SIZE_PAGE];
} FlashPage;

typedef struct
{
  union
  {
    uint8     byte[SERIAL_FLASH_SIZE_SUBSECTOR];
    FlashPage page[SF_PAGES_PER_SUBSECTOR];
  };
} FlashSubSector;

typedef struct
{
  union
  {
    uint8          byte[SERIAL_FLASH_SIZE_SECTOR];
    FlashPage      page[SF_PAGES_PER_SECTOR];
    FlashSubSector subSector[SF_SUBSECTORS_PER_SECTOR];
  };
} FlashSector;

typedef struct
{
  union
  {
    uint8          byte[SERIAL_FLASH_SIZE_CHIP];
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

void    SerialFlash_test(void);
void    SerialFlash_init(void);
FlashID SerialFlash_readFlashID(void);
uint32 SerialFlash_getStateAsWord(void);
double SerialFlash_getStateVoltage(void);
boolean SerialFlash_setup(boolean state);
SerialFlashState SerialFlash_getState(void);
boolean SerialFlash_setPowerProfile(SerialFlashPowerProfile profile);
boolean SerialFlash_setPowerState(SerialFlashState state, double vDomain);
SerialFlashResult SerialFlash_read(uint8 *pSrc, uint8 *pDest, uint16 length);
SerialFlashResult SerialFlash_write(uint8 *pSrc, uint8 *pDest, uint16 length);

#endif
