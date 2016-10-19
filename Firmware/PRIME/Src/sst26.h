#ifndef SST26_H
#define SST26_H

#include "types.h"

#pragma anon_unions

typedef enum
{
  SST26_STATE_IDLE    = 0,
  SST26_STATE_READING = 1,
  SST26_STATE_ERASING = 2,
  SST26_STATE_WRITING = 3,
  SST26_STATE_WAITING = 4,
  SST26_STATE_MAX     = 5
} SST26State;

typedef enum
{
  SST26_PROFILE_STANDARD = 0,
  SST26_PROFILE_30VIW    = 1,
  SST26_PROFILE_27VIW    = 2,
  SST26_PROFILE_23VIW    = 3,
  SST26_PROFILE_21VIW    = 4,
  SST26_PROFILE_MAX      = 5
} SST26PowerProfile;

typedef enum
{
  SST26_SIZE_PAGE      = 0x000100,
  SST26_SIZE_SUBSECTOR = 0x001000,
  SST26_SIZE_SECTOR    = 0x010000,
  SST26_SIZE_CHIP      = 0x100000
} SST26Size;

typedef enum
{
  SST26_RESULT_OK           = 0,
  SST26_RESULT_NEEDED_RETRY = 1,
  SST26_RESULT_ERROR        = 2
} SST26Result;

#define SST26_SECTORS_PER_CHIP      (SST26_SIZE_CHIP      / SST26_SIZE_SECTOR)
#define SST26_SUBSECTORS_PER_CHIP   (SST26_SIZE_CHIP      / SST26_SIZE_SUBSECTOR)
#define SST26_PAGES_PER_CHIP        (SST26_SIZE_CHIP      / SST26_SIZE_PAGE)
#define SST26_SUBSECTORS_PER_SECTOR (SST26_SIZE_SECTOR    / SST26_SIZE_SUBSECTOR)
#define SST26_PAGES_PER_SECTOR      (SST26_SIZE_SECTOR    / SST26_SIZE_PAGE)
#define SST26_PAGES_PER_SUBSECTOR   (SST26_SIZE_SUBSECTOR / SST26_SIZE_PAGE)

typedef struct
{
  uint8 byte[SST26_SIZE_PAGE];
} SST26Page;

typedef struct
{
  union
  {
    uint8     byte[SST26_SIZE_SUBSECTOR];
    SST26Page page[SST26_PAGES_PER_SUBSECTOR];
  };
} SST26SubSector;

typedef struct
{
  union
  {
    uint8          byte[SST26_SIZE_SECTOR];
    SST26Page      page[SST26_PAGES_PER_SECTOR];
    SST26SubSector subSector[SST26_SUBSECTORS_PER_SECTOR];
  };
} SST26Sector;

typedef struct
{
  union
  {
    uint8          byte[SST26_SIZE_CHIP];
    SST26Page      page[SST26_PAGES_PER_CHIP];
    SST26SubSector subSector[SST26_SUBSECTORS_PER_CHIP];
    SST26Sector    sector[SST26_SECTORS_PER_CHIP];
  };
} SST26Organization;

typedef struct
{
  SST26SubSector subSector;
} SST26Map;

#define gpSST26 ((const FlashMap*) 0)


typedef struct
{
  uint8  manufacturerId;
  uint8  memoryType;
  uint8  deviceId;
  uint8  reserved;
} SST26FlashId;

bool    SST26_test(void);
void    SST26_init(void);
SST26FlashId SST26_readSST26FlashId(void);
uint32_t *SST26_getStatePointer(void);
double  SST26_getStateVoltage(void);
boolean SST26_setup(boolean state);
boolean SST26_setPowerProfile(SST26PowerProfile profile);
boolean SST26_setPowerState(SST26State state, double vDomain);
SST26Result SST26_read(uint8 *pSrc, uint8 *pDest, uint32 length, bool fast);
SST26Result SST26_write(uint8 *pSrc, uint8 *pDest, uint32 len, OpDelays *pDelays);

#endif
