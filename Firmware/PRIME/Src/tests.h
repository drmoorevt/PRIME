#ifndef TESTS_H
#define TESTS_H

#include "types.h"
#include "analog.h"

void Tests_init(void);
void Tests_run(void);
void Tests_notifySampleTrigger(void);
void Tests_notifyConversionComplete(ADCPort port, uint32_t chan, uint32 numSamples);

#endif // TESTS_H
