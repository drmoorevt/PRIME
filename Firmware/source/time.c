#include "stm32f2xx.h"
#include "time.h"
#include "util.h"
#include "tests.h"

#define FILE_ID TIME_C

#define MILLISECONDS_PER_SECOND (1000)
#define MILLISECONDS_PER_MINUTE (MILLISECONDS_PER_SECOND * 60)
#define MILLISECONDS_PER_HOUR   (MILLISECONDS_PER_MINUTE * 60)
#define MILLISECONDS_PER_DAY    (MILLISECONDS_PER_HOUR   * 24)

#define ENABLE_SYSTICK_INTERRUPT()  do {NVIC_EnableIRQ(SysTick_IRQn);} while(0)
#define DISABLE_SYSTICK_INTERRUPT() do {NVIC_DisableIRQ(SysTick_IRQn);} while(0)

#define TIM_TRGOSource_Update              ((uint16_t)0x0020)

struct
{
  SoftTimerConfig softTimers[TIME_SOFT_TIMER_MAX];
  uint64 systemTime;  // milliseconds since the epoch
  boolean isSecondBoundary;
} sTime;

static void Time_initSysTick(void);
static void Time_decrementSoftTimers(void);

/**************************************************************************************************\
* FUNCTION      Time_init
* DESCRIPTION   Initializes the 1ms SysTick and clears all software timers
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_init(void)
{
  Util_fillMemory((uint8*)&sTime, sizeof(sTime), 0x00);  // Clear all of the software timers
  Time_initSysTick();
}

/**************************************************************************************************\
* FUNCTION      Time_stopTimer
* DESCRIPTION
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_stopTimer(HardTimer timer)
{
  switch (timer)
  {
    case TIME_HARD_TIMER_TIMER1:
      TIM1->CR1 &= (~TIM_CR1_CEN);
      break;
    case TIME_HARD_TIMER_TIMER2:
      TIM2->CR1 &= (~TIM_CR1_CEN);
      break;
    case TIME_HARD_TIMER_TIMER3:
      TIM3->CR1 &= (~TIM_CR1_CEN);
      break;
  }
}

/**************************************************************************************************\
* FUNCTION      Time_initTimer1
* DESCRIPTION   Initializes timer1
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_initTimer1(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // Turn on Timer1 clocks (120 MHz)
  TIM1->ARR     = 120; // Autoreload set to 1us overflow
  TIM1->CR1    |= (TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_ARPE);
  TIM1->CR2     = (0x00000000);
  TIM1->DIER   |= TIM_DIER_UIE; // Turn on the timer (update) interrupt
  NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
}

/**************************************************************************************************\
* FUNCTION    Time_timer1IRQ
* DESCRIPTION Initializes timer1
* PARAMETERS  none
* RETURN        none
\**************************************************************************************************/
void TIM1_UP_TIM10_IRQHandler(void)
{
  CLEAR_BIT(TIM2->SR, TIM_SR_UIF); // Clear the update interrupt flag
}

/**************************************************************************************************\
* FUNCTION      Time_initTimer2
*  DESCRIPTION    Initializes timer2
* PARAMETERS    reloadValue: The automatic reload value, when reached an IRQ is triggered
* RETURN        none
\**************************************************************************************************/
void Time_initTimer2(uint16 reloadValue)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Turn on Timer2 clocks (60 MHz)
  TIM2->ARR     = reloadValue;
  TIM2->CR1    |= (TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_ARPE);
  TIM2->CR2     = (0x00000000);
  TIM2->DIER   |= TIM_DIER_UIE; // Turn on the timer (update) interrupt
  NVIC_EnableIRQ(TIM2_IRQn);
}

/**************************************************************************************************\
* FUNCTION      TIM2_IRQHandler
* DESCRIPTION   Handles interrupts originating from Timer2
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void TIM2_IRQHandler(void)
{
  CLEAR_BIT(TIM2->SR, TIM_SR_UIF); // Clear the update interrupt flag
}

/**************************************************************************************************\
* FUNCTION      Time_initTimer3
* DESCRIPTION   Initializes timer3
* PARAMETERS    reloadValue: The automatic reload value, when reached an IRQ is triggered
* RETURN        none
\**************************************************************************************************/
void Time_initTimer3(uint16 reloadValue)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // Turn on Timer3 clocks (60 MHz)
  TIM3->ARR     = reloadValue;
  TIM3->CR1    |= (TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_ARPE);
  TIM3->CR2    &= (~TIM_CR2_MMS);
  TIM3->CR2    |= (TIM_TRGOSource_Update);
  TIM3->DIER   |= TIM_DIER_UIE; // Turn on the timer (update) interrupt
  NVIC_EnableIRQ(TIM3_IRQn);
}

/**************************************************************************************************\
* FUNCTION      TIM3_IRQHandler
* DESCRIPTION   Handles interrupts originating from Timer3
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void TIM3_IRQHandler(void)
{
  TIM3->EGR = TIM_EGR_TG;
  CLEAR_BIT(TIM3->SR, TIM_SR_UIF); // Clear the update interrupt flag
  Tests_notifySampleTrigger();
}

/**************************************************************************************************\
* FUNCTION      Time_startTimer
* DESCRIPTION   Creates a timer from the supplied configuration
* PARAMETERS    timerConfig - the configuration for the timer to use
* RETURN        None -- (uses the configured callback if supplied)
\**************************************************************************************************/
void Time_startTimer(SoftTimerConfig timerConfig)
{
  DISABLE_SYSTICK_INTERRUPT();
  sTime.softTimers[timerConfig.timer] = timerConfig;
  ENABLE_SYSTICK_INTERRUPT();
}

/**************************************************************************************************\
* FUNCTION      Time_delay
* DESCRIPTION   Blocking delay
* PARAMETERS    microSeconds - number of microSeconds to delay
* RETURN        none
* NOTES         Timer5 is connected to APB1 which is operating at 60MHz
\**************************************************************************************************/
void Time_delay(uint32 microSeconds)
{
  if (0 == microSeconds)
    return;
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  // Turn on Timer5 clocks (60 MHz)
  TIM5->CR1     = (0x0000);            // Turn off the counter entirely
  TIM5->PSC     = (60);                // Set prescalar to 60. Timer operates at 60MHz.
  TIM5->ARR     = (microSeconds);      // Set up the counter to count down
  TIM5->SR      = (0x0000);            // Clear all status (and interrupt) bits
  TIM5->DIER    = (0x0000);            // Turn off the timer (update) interrupt
  TIM5->CR2     = (0x0000);            // Ensure CR2 is at default settings
  TIM5->CR1     = (0x0000) | (TIM_CR1_CEN   | // Turn on the timer
                              TIM_CR1_URS   | // Only overflow/underflow generates an update IRQ
                              TIM_CR1_OPM);   // One pulse mode, disable after count hits zero
  while (!(TIM5->SR & TIM_SR_UIF));           // Wait until timer hits the ARR value
}

/**************************************************************************************************\
* FUNCTION      Time_getTimerValue
* DESCRIPTION   Blocking delay
* PARAMETERS    timer - index of timer
* RETURN        uint32 - number of subticks left on the timer
\**************************************************************************************************/
uint32 Time_getTimerValue(SoftTimer timer)
{
  uint32 value;
  DISABLE_SYSTICK_INTERRUPT();
  value = sTime.softTimers[timer].value;
  ENABLE_SYSTICK_INTERRUPT();
  return value;
}

/**************************************************************************************************\
* FUNCTION      Time_getSystemTime
* DESCRIPTION   Gets system time as a 32 bit UTC since Jan. 1, 1970 value (secs)
* PARAMETERS    none
* RETURN        uint32 - systemTime
\**************************************************************************************************/
uint32 Time_getSystemTime(void)
{
  uint32 sysTime;
  
  DISABLE_SYSTICK_INTERRUPT();
  sysTime = sTime.systemTime;
  ENABLE_SYSTICK_INTERRUPT();

  return sysTime;
}
/**************************************************************************************************\
* FUNCTION      Time_getTimeOfday
* DESCRIPTION   Gets the milliseconds time within the day
* PARAMETERS    none
* RETURN        uint32 - time
\**************************************************************************************************/
uint32 Time_getTimeOfday(void)
{
  uint32 timeOfDay;
  DISABLE_SYSTICK_INTERRUPT();
  timeOfDay = sTime.systemTime % MILLISECONDS_PER_DAY;
  ENABLE_SYSTICK_INTERRUPT();
  return timeOfDay;
}

/**************************************************************************************************\
* FUNCTION      Time_isSecondBoundary
* DESCRIPTION   Checks to see if we have crossed a second boundary.  This clears the boundary flag
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
boolean Time_isSecondBoundary(void)
{
  boolean isSecondBoundary = sTime.isSecondBoundary;
  sTime.isSecondBoundary   = FALSE;
  return isSecondBoundary;
}

/**************************************************************************************************\
* FUNCTION      Time_decrementSoftTimers
* DESCRIPTION   Decrements all timers that aren't already 0
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
static void Time_decrementSoftTimers(void)
{
  uint8 i;
  for (i = 0; i < TIME_SOFT_TIMER_MAX; i++)
  {
    if (sTime.softTimers[i].value > 0)
    {
      if (1 == sTime.softTimers[i].value--)
      { // Timer just expired? Notify the app if configured.
        if (NULL != sTime.softTimers[i].appNotifyTimerExpired)
          sTime.softTimers[i].appNotifyTimerExpired((SoftTimer)i);
        // Reload the timer if configured for auto-reload
        sTime.softTimers[i].value = sTime.softTimers[i].reload;
      }
    }
  }
}

/**************************************************************************************************\
* FUNCTION      SysTick_Handler
* DESCRIPTION   Interrupt that fires on the 1ms system tick
* PARAMETERS    none
* RETURN        none
* NOTES         Called in interrupt to decrement timers, handle time/date functionality
\**************************************************************************************************/
void SysTick_Handler(void)
{
  if (0 == ((sTime.systemTime++) % MILLISECONDS_PER_SECOND))
    sTime.isSecondBoundary = TRUE;
  Time_decrementSoftTimers();
}

/**************************************************************************************************\
* FUNCTION      Time_initSysTick
* DESCRIPTION   Initializes the 1ms system tick timer
* PARAMETERS    none
* RETURN        none
* NOTES         These
\**************************************************************************************************/
static void Time_initSysTick(void)
{
  SysTick_Config(SystemCoreClock / MILLISECONDS_PER_SECOND); // div 1000 = 1ms tick
}
