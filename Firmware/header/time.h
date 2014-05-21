#ifndef TIME_H
#define TIME_H

#include "types.h"

typedef enum
{
  TIME_SOFT_TIMER_DELAY               = 0, // Time_coarseDelay timer
  TIME_SOFT_TIMER_UART                = 1, // used for protocol timeouts
  TIME_SOFT_TIMER_SERIAL_MEM          = 2, // eeprom/flash timer
  TIME_SOFT_TIMER_UART_INTERCHAR      = 3, // interchar uart timeout
  TIME_SOFT_TIMER_XBEE_UART_INTERCHAR = 4, // xbee interchar uart timeout
  TIME_SOFT_TIMER_ANALOG              = 5, // reserving timer for analog module
  TIME_SOFT_TIMER_XBEE                = 6, // generic xbee timer
  TIME_SOFT_TIMER_XBEE_READ           = 7, // xbee read timeout
  TIME_SOFT_TIMER_ONE_SECOND          = 8, // One second timer
  TIME_SOFT_TIMER_MAX                 = 9
} SoftTimer;

typedef enum
{
  TIME_HARD_TIMER_TIMER0  = 0,
  TIME_HARD_TIMER_TIMER1  = 1,  // Used for fine delays
  TIME_HARD_TIMER_TIMER2  = 2,
  TIME_HARD_TIMER_TIMER3  = 3,  // Triggers analog sampling
  TIME_HARD_TIMER_TIMER4  = 4,
  TIME_HARD_TIMER_TIMER5  = 5,
  TIME_HARD_TIMER_TIMER6  = 6,
  TIME_HARD_TIMER_TIMER7  = 7,
  TIME_HARD_TIMER_TIMER8  = 8,
  TIME_HARD_TIMER_TIMER9  = 9,
  TIME_HARD_TIMER_TIMER10 = 10,
  TIME_HARD_TIMER_TIMER11 = 11,
  TIME_HARD_TIMER_TIMER12 = 12,
  TIME_HARD_TIMER_TIMER13 = 13,
  TIME_HARD_TIMER_TIMER14 = 14,
  TIME_HARD_TIMER_MAX = 15
} HardTimer;

void Time_init(void);
void Time_stopTimer(HardTimer timer);
void Time_initTimer2(uint16 reloadValue);
void Time_initTimer3(uint16 reloadValue);
void Time_startSoftTimer(SoftTimer timer, uint32 milliSeconds);
void Time_fineDelay(uint32 microSeconds);
void Time_coarseDelay(uint32 numSubTicks);
uint16 Time_getTimerValue(SoftTimer timer);
uint32 Time_getSystemTime(void);
uint32 Time_getTimeOfday(void);
void Time_handleSubTick(void);
void Time_incrementSystemTime(void);
boolean Time_isSecondBoundary(void);

#endif //TIME_H
