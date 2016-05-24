#ifndef PLR5010D_H_
#define PLR5010D_H_

#include "types.h"

typedef enum
{
  PLR5010D_DOMAIN0 = 0x00,
  PLR5010D_DOMAIN1 = 0x01,
  PLR5010D_DOMAIN2 = 0x02
} PLR5010DSelect;

bool PLR5010D_init(void);
bool PLR5010D_test(PLR5010DSelect select);
bool PLR5010D_setVoltage(PLR5010DSelect device, uint16 outBitsA, uint16 outBitsB);

#endif /* PLR5010D_H_ */
