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
  SDCARD_IDLE,
  SDCARD_SETUP,
  SDCARD_READY,
  SDCARD_READING,
  SDCARD_WRITING,
  SDCARD_VERIFYING
} SDCardState;

void    SDCard_init(void);
void    SDCard_test(void);
boolean SDCard_setup(void);
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_writeLP(uint8 *pSrc, uint8 *pDest, uint16 length);
boolean SDCard_writeXLP(uint8 *pSrc, uint8 *pDest, uint16 length);
SDCardState SDCard_getState(void);

#endif
