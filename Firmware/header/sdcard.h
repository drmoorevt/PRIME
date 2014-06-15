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
  SDCARD_STATE_VERIFYING  = 5,
  SDCARD_STATE_MAX        = 6
} SDCardState;

typedef enum
{
  SDCARD_PROFILE_STANDARD    = 0,
  SDCARD_PROFILE_LP_WAIT     = 1,
  SDCARD_PROFILE_LP_ALL      = 2,
  SDCARD_PROFILE_XLP_WAIT    = 3,
  SDCARD_PROFILE_LP_XLP_WAIT = 4,
  SDCARD_PROFILE_MAX         = 5
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

void    SDCard_init(void);
boolean SDCard_setup(boolean state);
boolean SDCard_initDisk(void);
void SDCard_notifyVoltageChange(double newVoltage);
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length);
SDWriteResult SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_setPowerState(SDCardState state, double vDomain);
boolean SDCard_setPowerProfile(SDCardPowerProfile profile);
SDCardState SDCard_getState(void);
void    SDCard_test(void);

#endif
