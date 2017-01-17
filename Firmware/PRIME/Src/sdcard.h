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
  SDCARD_STATE_IDLE       = 0,
  SDCARD_STATE_SETUP      = 1,
  SDCARD_STATE_READY      = 2,
  SDCARD_STATE_READING    = 3,
  SDCARD_STATE_WRITING    = 4,
  SDCARD_STATE_WAITING    = 5,
  SDCARD_STATE_VERIFYING  = 6,
  SDCARD_STATE_MAX        = 7
} SDCardState;

typedef enum
{
  SDCARD_PROFILE_STANDARD = 0,
  SDCARD_PROFILE_30VISR   = 1,
  SDCARD_PROFILE_27VISR   = 2,
  SDCARD_PROFILE_24VISR   = 3,
  SDCARD_PROFILE_21VISR   = 4,
  SDCARD_PROFILE_MAX      = 5
} SDCardPowerProfile;

typedef enum
{
  SD_WRITE_RESULT_OK            = 0,
  SD_WRITE_RESULT_READ_FAILED   = 1,
  SD_WRITE_RESULT_WRITE_FAILED  = 2,
  SD_WRITE_RESULT_VERIFY_FAILED = 3,
  SD_WRITE_RESULT_BAD_COMMAND   = 4,
  SD_WRITE_RESULT_NOT_READY     = 5,
  SDCARD_RESULT_MAX
} SDWriteResult;

typedef struct
{
  uint32 cmdWaitClocks;
  uint32 writeWaitClocks;
  uint32 preReadWaitClocks;
  uint32 postReadWaitClocks;
  uint32 preWriteWaitClocks;
  uint32 postWriteWaitClocks;
} SDCardTiming;

SDCardTiming SDCard_getLastTiming(void);

void    SDCard_init(void);
boolean SDCard_setup(boolean state);
boolean SDCard_initDisk(void);
void SDCard_notifyVoltageChange(double newVoltage);
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length);
SDWriteResult SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length, OpDelays *pDelays);
boolean SDCard_setPowerState(SDCardState state, double vDomain);
boolean SDCard_setPowerProfile(SDCardPowerProfile profile);
uint32_t *SDCard_getStatePointer(void);
double SDCard_getStateVoltage(void);
bool    SDCard_test(void);

#endif
