#ifndef PLR5010D_H_
#define PLR5010D_H_

#include "types.h"

typedef enum
{
  PLR5010D_DOMAIN0 = 0x00,
  PLR5010D_DOMAIN1 = 0x01,
  PLR5010D_DOMAIN2 = 0x02
} PLR5010DSelect;

typedef enum
{
  PLR5010D_CHANA = 0x00,
  PLR5010D_CHANB = 0x01
} PLR5010DChannel;

bool PLR5010D_init(void);
bool PLR5010D_test(PLR5010DSelect device);
bool PLR5010D_setVoltage(PLR5010DSelect device, PLR5010DChannel chan, double volts);

#endif /* PLR5010D_H_ */
