#ifndef DAC_H
#define DAC_H

#include "types.h"

typedef enum
{
  DAC_PORT1,
  DAC_PORT2
} DACPort;

void DAC_init(void);
void DAC_setDAC(DACPort port, uint16 outVal);
void DAC_setVoltage(DACPort port, float outVolts);

#endif // DAC_H
