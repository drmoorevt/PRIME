#ifndef TIME_H
#define TIME_H

#include "types.h"

typedef enum
{
  TIME_SOFT_TIMER_DELAY               =  0, // Used for 1ms resolution, non-blocking delays
  TIME_SOFT_TIMER_USART1              =  1, // USART1 protocol timeouts
  TIME_SOFT_TIMER_USART2              =  2, // USART2 protocol timeouts
  TIME_SOFT_TIMER_USART3              =  3, // USART3 protocol timeouts
  TIME_SOFT_TIMER_UART4               =  4, // UART4 protocol timeouts
  TIME_SOFT_TIMER_UART5               =  5, // UART5 protocol timeouts
  TIME_SOFT_TIMER_USART6              =  6, // USART6 protocol timeouts
  TIME_SOFT_TIMER_SERIAL_MEM          =  7, // eeprom/flash timer
  TIME_SOFT_TIMER_UART_INTERCHAR      =  8, // interchar uart timeout
  TIME_SOFT_TIMER_XBEE_UART_INTERCHAR =  9, // xbee interchar uart timeout
  TIME_SOFT_TIMER_ANALOG              = 10, // reserving timer for analog module
  TIME_SOFT_TIMER_XBEE                = 11, // generic xbee timer
  TIME_SOFT_TIMER_XBEE_READ           = 12, // xbee read timeout
  TIME_SOFT_TIMER_ONE_SECOND          = 13, // One second timer
  TIME_SOFT_TIMER_USB_RX              = 14, // USB receive timer
  TIME_SOFT_TIMER_USB_TX              = 15, // USB transmit timer
  TIME_SOFT_TIMER_MAX                 = 16
} SoftTimer;

typedef enum
{
  TIME_HARD_TIMER_TIMER0  = 0,
  TIME_HARD_TIMER_TIMER1  = 1,
  TIME_HARD_TIMER_TIMER2  = 2,
  TIME_HARD_TIMER_TIMER3  = 3,  // Triggers analog sampling
  TIME_HARD_TIMER_TIMER4  = 4,
  TIME_HARD_TIMER_TIMER5  = 5,  // Used for fine delays
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

typedef struct
{
  SoftTimer timer;  // One of the defined SoftTimers
  uint32    value;  // Initial timer value in milliseconds
  uint32    reload; // One shot if reload is zero
  void      (*appNotifyTimerExpired)(SoftTimer timer); // Can be null
} SoftTimerConfig;

typedef struct
{
  uint32_t tDelay;
  uint32_t eDelay;
  uint32_t cDelay;
} Delay;

#define MAX_NUM_DELAYS 3

typedef struct
{
  Delay op[MAX_NUM_DELAYS];
} OpDelays;

#define ENABLE_SYSTICK()  SysTick->CTRL = SysTick->CTRL | (SysTick_CTRL_ENABLE_Msk)
#define DISABLE_SYSTICK() SysTick->CTRL = SysTick->CTRL & (~SysTick_CTRL_ENABLE_Msk)
#define ENABLE_SYSTICK_INTERRUPT()  SysTick->CTRL = SysTick->CTRL | (SysTick_CTRL_TICKINT_Msk)
#define DISABLE_SYSTICK_INTERRUPT() SysTick->CTRL = SysTick->CTRL & (~SysTick_CTRL_TICKINT_Msk)

void Time_init(void);
void Time_delay(uint32 microSeconds);

void Time_startTimer(SoftTimerConfig timerConfig);
uint32 Time_getTimerValue(SoftTimer timer);

uint64_t Time_pendEnergyTime(Delay *pDelay);

#endif //TIME_H
