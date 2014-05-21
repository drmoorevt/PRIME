#include "stm32f2xx.h"
#include "time.h"
#include "util.h"
#include "tests.h"

#define FILE_ID TIME_C

#define MILLISECONDS_PER_SECOND (1000)
#define MILLISECONDS_PER_MINUTE (MILLISECONDS_PER_SECOND * 60)
#define MILLISECONDS_PER_HOUR   (MILLISECONDS_PER_MINUTE * 60)
#define MILLISECONDS_PER_DAY    (MILLISECONDS_PER_HOUR   * 24)

#define ENABLE_HARDTIMER_INTERRUPT()  do {TIM1->DIER |=  TIM_DIER_UIE;} while(0)
#define DISABLE_HARDTIMER_INTERRUPT() do {TIM1->DIER &= ~TIM_DIER_UIE;} while(0)

#define ENABLE_SYSTICK_INTERRUPT()  do {SysTick->CTRL |=  SysTick_CTRL_TICKINT_Msk;} while(0)
#define DISABLE_SYSTICK_INTERRUPT() do {SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;} while(0)

struct
{
  uint32 softTimers[TIME_SOFT_TIMER_MAX];
  uint64 systemTime;                      // milliseconds since the epoch
  boolean isSecondBoundary;
} sTime;

void Time_initSysTick(void);
void Time_decrementSoftTimers(void);

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
//  TIM3->SR      = 0;
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
* FUNCTION      Time_startSoftTimer
* DESCRIPTION   Initializes timers
* PARAMETERS    timer - index of timer
*               milliSeconds - number of milliseconds to run timer
* RETURN        none
\**************************************************************************************************/
void Time_startSoftTimer(SoftTimer timer, uint32 milliSeconds)
{
  DISABLE_SYSTICK_INTERRUPT(); 
  sTime.softTimers[timer] = milliSeconds;
  ENABLE_SYSTICK_INTERRUPT();
}

/**************************************************************************************************\
* FUNCTION      Time_coarseDelay
* DESCRIPTION   Blocking delay
* PARAMETERS    numSubTicks - number of subticks to delay
* RETURN        none
\**************************************************************************************************/
void Time_coarseDelay(uint32 milliSeconds)
{
  Time_startSoftTimer(TIME_SOFT_TIMER_DELAY, milliSeconds);
  while (Time_getTimerValue(TIME_SOFT_TIMER_DELAY));
}

/**************************************************************************************************\
* FUNCTION      Time_fineDelay
* DESCRIPTION   Blocking delay
* PARAMETERS    numTicks - number of ticks to delay
* RETURN        none
* NOTES         Timer5 is connected to APB1 which is operating at 60MHz
\**************************************************************************************************/
void Time_fineDelay(uint32 microSeconds)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  // Turn on Timer5 clocks (60 MHz)
  TIM5->CR1     = (0x0000);            // Turn off the counter entirely
  TIM5->PSC     = (60);                // Set prescalar to 60. Timer operates at 60MHz.
  TIM5->CNT     = (microSeconds);      // Set up the counter to count down
  TIM5->ARR     = (microSeconds);      // Set up the counter to count down
  TIM5->SR      = (0x0000);            // Clear all status (and interrupt) bits
  TIM5->DIER    = (0x0000);            // Turn off the timer (update) interrupt
  TIM5->CR2     = (0x0000) | (TIM_TRGOSource_Update); // Update event is the trigger output
  TIM5->CR1     = (0x0000) | (TIM_CR1_CEN   | // Turn on the timer
                              TIM_CR1_DIR   | // Make the timer count down
                              TIM_CR1_URS   | // Only overflow/underflow generates an update IRQ
                              TIM_CR1_UDIS  | // Disable update event generation
                              TIM_CR1_ARPE  | // ARR register is buffered
                              TIM_CR1_OPM);   // One pulse mode, disable after count hits zero
  while (TIM5->CNT);                          // Wait until timer count hits zero
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
  value = sTime.softTimers[timer];
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
* FUNCTION      Time_incrementSystemTime
*  DESCRIPTION    Adds 1 second to the system time
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_incrementSystemTime(void)
{
  sTime.systemTime++;
}

/**************************************************************************************************\
* FUNCTION      Time_decrementSoftTimers
*  DESCRIPTION    Decrements all timers that aren't already 0
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_decrementSoftTimers(void)
{
  uint8 i;
  for (i = 0; i < TIME_SOFT_TIMER_MAX; i++)
  {
    if(sTime.softTimers[i] > 0)
      sTime.softTimers[i]--;
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
