#ifndef TIME_H
#define TIME_H

#include "types.h"

#define TIME_ONE_SECOND_IN_SUBTICKS         ((uint8) 128)
#define TIME_ONE_HUNDRED_MILLIS_IN_SUBTICKS ((uint8) 13)

typedef enum
{
  TIMER_DELAY = 0,            // Time_delay timer
  TIMER_UART,                 // used for protocol timeouts
  TIMER_SERIAL_MEM,           // eeprom/flash timer
  TIMER_UART_INTERCHAR,       // c12.18 interchar uart timeout
  TIMER_XBEE_UART_INTERCHAR,  // xbee interchar uart timeout
  TIMER_RUN_HEALTH,           // health timer
  TIMER_ADE,                  // timer used with ade metering chip inf
  TIMER_XBEE,                 // generic xbee timer
  TIMER_XBEE_READ,            // xbee read timeout
  TOTAL_NUM_TIMERS            
} SoftTimer;

typedef enum
{
  TIMER0  = 0,
  TIMER1  = 1,
  TIMER2  = 2,
  TIMER3  = 3,
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

void Time_initTimer1(void);
void Time_initTimer2(void);
void Time_startTimer(SoftTimer timer, uint16 numSubTicks);
void Time_delay(uint16 numSubTicks);
uint16 Time_getTimerValue(SoftTimer timer);
uint32 Time_getSystemTime(void);
uint32 Time_getTimeOfday(void);
void Time_handleSubTick(void);
void Time_incrementSystemTime(void);
boolean Time_isSecondBoundary(void);

#endif //TIME_H
