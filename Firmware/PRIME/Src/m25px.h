#ifndef M25PX_H
#define M25PX_H

#include "types.h"

#pragma anon_unions

typedef enum
{
  M25PX_STATE_IDLE    = 0,
  M25PX_STATE_READING = 1,
  M25PX_STATE_ERASING = 2,
  M25PX_STATE_WRITING = 3,
  M25PX_STATE_WAITING = 4,
  M25PX_STATE_MAX     = 5
} M25PXState;

typedef enum
{
  M25PX_PROFILE_STANDARD = 0,
  M25PX_PROFILE_30VIW    = 1,
  M25PX_PROFILE_27VIW    = 2,
  M25PX_PROFILE_23VIW    = 3,
  M25PX_PROFILE_21VIW    = 4,
  M25PX_PROFILE_MAX      = 5
} M25PXPowerProfile;

typedef enum
{
  M25PX_SIZE_PAGE      = 0x000100,
  M25PX_SIZE_SUBSECTOR = 0x001000,
  M25PX_SIZE_SECTOR    = 0x010000,
  M25PX_SIZE_CHIP      = 0x100000
} M25PXSize;

typedef enum
{
  M25PX_RESULT_OK           = 0,
  M25PX_RESULT_NEEDED_RETRY = 1,
  M25PX_RESULT_ERROR        = 2
} M25PXResult;

#define M25PX_SECTORS_PER_CHIP      (M25PX_SIZE_CHIP      / M25PX_SIZE_SECTOR)
#define M25PX_SUBSECTORS_PER_CHIP   (M25PX_SIZE_CHIP      / M25PX_SIZE_SUBSECTOR)
#define M25PX_PAGES_PER_CHIP        (M25PX_SIZE_CHIP      / M25PX_SIZE_PAGE)
#define M25PX_SUBSECTORS_PER_SECTOR (M25PX_SIZE_SECTOR    / M25PX_SIZE_SUBSECTOR)
#define M25PX_PAGES_PER_SECTOR      (M25PX_SIZE_SECTOR    / M25PX_SIZE_PAGE)
#define M25PX_PAGES_PER_SUBSECTOR   (M25PX_SIZE_SUBSECTOR / M25PX_SIZE_PAGE)

typedef struct
{
  uint8 byte[M25PX_SIZE_PAGE];
} M25PXPage;

typedef struct
{
  union
  {
    uint8     byte[M25PX_SIZE_SUBSECTOR];
    M25PXPage page[M25PX_PAGES_PER_SUBSECTOR];
  };
} M25PXSubSector;

typedef struct
{
  union
  {
    uint8          byte[M25PX_SIZE_SECTOR];
    M25PXPage      page[M25PX_PAGES_PER_SECTOR];
    M25PXSubSector subSector[M25PX_SUBSECTORS_PER_SECTOR];
  };
} M25PXSector;

typedef struct
{
  union
  {
    uint8          byte[M25PX_SIZE_CHIP];
    M25PXPage      page[M25PX_PAGES_PER_CHIP];
    M25PXSubSector subSector[M25PX_SUBSECTORS_PER_CHIP];
    M25PXSector    sector[M25PX_SECTORS_PER_CHIP];
  };
} M25PXOrganization;

typedef struct
{
  M25PXSubSector subSector;
} M25PXMap;

typedef struct
{
  uint8  manufacturerId;
  uint8  memoryType;
  uint8  memoryCapacity;
  uint8  uidLength;
  uint8  uniqueId[16];
} M25PXFlashId;

bool    M25PX_test(void);
void    M25PX_init(void);
M25PXFlashId M25PX_readM25PXFlashId(void);
uint32_t *M25PX_getStatePointer(void);
double  M25PX_getStateVoltage(void);
boolean M25PX_setup(boolean state);
M25PXState M25PX_getState(void);
boolean M25PX_setPowerProfile(M25PXPowerProfile profile);
boolean M25PX_setPowerState(M25PXState state, double vDomain);
M25PXResult M25PX_read(uint8 *pSrc, uint8 *pDest, uint16 length);
M25PXResult M25PX_write(uint8 *pSrc, uint8 *pDst, uint32 len, OpDelays *pDelays);

#endif
