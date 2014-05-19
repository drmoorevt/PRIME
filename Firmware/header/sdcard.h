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

void    SDCard_init(void);
boolean SDCard_setup(boolean state);
boolean SDCard_initDisk(void);
void SDCard_notifyVoltageChange(double newVoltage);
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_setPowerState(SDCardState state, double vDomain);
SDCardState SDCard_getState(void);
void    SDCard_test(void);

#endif
