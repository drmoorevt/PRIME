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
  SDCARD_IDLE       = 0,
  SDCARD_SETUP      = 1,
  SDCARD_READY      = 2,
  SDCARD_READING    = 3,
  SDCARD_WRITING    = 4,
  SDCARD_VERIFYING  = 5,
  SDCARD_NUM_STATES = 6
} SDCardState;

void    SDCard_init(void);
boolean SDCard_setup(boolean state);
boolean SDCard_initDisk(void);
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length);
SDCardState SDCard_getState(void);
void    SDCard_test(void);

#endif
