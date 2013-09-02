#ifndef TIME_H
#define TIME_H

#include "types.h"

typedef enum
{
  TIMER_DELAY = 0,            // Time_delay timer
  TIMER_UART,                 // used for protocol timeouts
  TIMER_SERIAL_MEM,           // eeprom/flash timer
  TIMER_UART_INTERCHAR,       // interchar uart timeout
  TIMER_XBEE_UART_INTERCHAR,  // xbee interchar uart timeout
  TIMER_ADE,                  // timer used with ade metering chip inf
  TIMER_XBEE,                 // generic xbee timer
  TIMER_XBEE_READ,            // xbee read timeout
  TIMER_ONE_SECOND,           // One second timer
  TOTAL_NUM_TIMERS            
} SoftTimer;

typedef enum
{
  TIMER0  = 0,
  TIMER1  = 1,
  TIMER2  = 2,
  TIMER3  = 3,  // Triggers analog sampling
  TIMER4  = 4,
  TIMER5  = 5,
  TIMER6  = 6,
  TIMER7  = 7,
  TIMER8  = 8,
  TIMER9  = 9,
  TIMER10 = 10,
  TIMER11 = 11,
  TIMER12 = 12,
  TIMER13 = 13,
  TIMER14 = 14
} HardTimer;

void Time_init(void);
void Time_stopTimer(HardTimer timer);
void Time_initTimer1(void);
void Time_initTimer2(uint16 reloadValue);
void Time_initTimer3(uint16 reloadValue);
void Time_startTimer(SoftTimer timer, uint16 numSubTicks);
void Time_delay(uint16 numSubTicks);
uint16 Time_getTimerValue(SoftTimer timer);
uint32 Time_getSystemTime(void);
uint32 Time_getTimeOfday(void);
void Time_handleSubTick(void);
void Time_incrementSystemTime(void);
boolean Time_isSecondBoundary(void);

#endif //TIME_H
