#include "stm32f4xx_hal.h"
#include "time.h"
#include "util.h"
#include "tests.h"

#define FILE_ID TIME_C

struct
{
  SoftTimerConfig softTimers[TIME_SOFT_TIMER_MAX];
  volatile uint64_t pendEnergy;
  volatile uint64_t accumEnergy;
  volatile uint32_t energyIdx;
} sTime;

static void Time_decrementSoftTimers(void);

/**************************************************************************************************\
* FUNCTION      Time_notifyEnergyExpended
* DESCRIPTION   
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
bool Time_notifyEnergyExpended(uint32_t energyExpendedBitCounts)
{
  sTime.pendEnergy = (sTime.pendEnergy > energyExpendedBitCounts) 
                   ? (sTime.pendEnergy - energyExpendedBitCounts)
                   : (0);
  return sTime.pendEnergy > 0;
}

/**************************************************************************************************\
* FUNCTION      Time_init
* DESCRIPTION   Initializes the 1ms SysTick and clears all software timers
* PARAMETERS    none
* RETURN        none
\**************************************************************************************************/
void Time_init(void)
{
  Util_fillMemory((uint8*)&sTime, sizeof(sTime), 0x00);  // Clear all of the software timers
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
void Time_delay(volatile uint32 microSeconds)
{
  if (0 == microSeconds)
    return;
  const uint32_t apb1Prescalar = 4;
  const uint32_t timerMult = 2;
  uint32_t timFreq = (HAL_RCC_GetPCLK1Freq() / timerMult) * apb1Prescalar;
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;          // Turn on Timer5 clocks (90 MHz)
  TIM5->CR1     = (0x0000);                    // Turn off the counter entirely
  TIM5->PSC     = (timFreq / (1000 * 1000));   // Set prescalar to count up on microsecond bounds.
  TIM5->ARR     = (microSeconds);               // Set up the counter to count down
  TIM5->SR      = (0x0000);                    // Clear all status (and interrupt) bits
  TIM5->DIER    = (0x0000);                    // Turn off the timer (update) interrupt
  TIM5->CR2     = (0x0000);                    // Ensure CR2 is at default settings
  TIM5->CR1     = (0x0000) | (TIM_CR1_CEN   |  // Turn on the timer
                              TIM_CR1_URS   |  // Only overflow/underflow generates an update IRQ
                              TIM_CR1_OPM   |  // One pulse mode, disable after count hits zero
                              TIM_CR1_ARPE);   // Buffer the ARPE
  while (!(TIM5->SR & TIM_SR_UIF));            // Wait until timer hits the ARR value
}

bool Time_accumulateEnergy(uint32_t maxNum)
{
  while ((0xFFFF != GPSDRAM->samples[SDRAM_CHANNEL_OUTCURRENT][sTime.energyIdx]) && (maxNum--))
  {
    sTime.accumEnergy += GPSDRAM->samples[SDRAM_CHANNEL_OUTCURRENT][sTime.energyIdx];
    if ((sTime.accumEnergy > sTime.pendEnergy) || (sTime.energyIdx >= TESTS_MAX_SAMPLES))
      return true;
    else
      sTime.energyIdx++;
  }
  return false;
}

/**************************************************************************************************\
* FUNCTION      Time_pendEnergyTime
* DESCRIPTION   Blocking delay
* PARAMETERS    microSeconds - number of microSeconds to delay
* RETURN        none
* NOTES         Timer5 is connected to APB1 which is operating at 60MHz
\**************************************************************************************************/
void Time_pendEnergyTime(Delay *pDelay)
{
  if ((0 == pDelay->eDelay) && (0 == pDelay->tDelay))
    return;
  
  // if this is called waiting on zero microjoules then assume no actual energy wait
  if (pDelay->eDelay > 0)
    sTime.pendEnergy = pDelay->eDelay;
  else
    sTime.pendEnergy = 0xFFFFFFFFFFFFFFFF;
  
  sTime.accumEnergy = 0;
  sTime.energyIdx = 0;
  
  // if this is called waiting only on energy, then assume no time wait
  if (pDelay->tDelay > 0)
  {
    uint32_t timFreq = HAL_RCC_GetPCLK1Freq();   // TIM5 is connected to APB1
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;          // Turn on Timer5 clocks (45 MHz)
    TIM5->CR1     = (0x0000);                    // Turn off the counter entirely
    TIM5->PSC     = (timFreq / (1000 * 1000));   // Set prescalar to count up on microsecond bounds.
    TIM5->ARR     = (pDelay->tDelay);               // Set up the counter to count down
    TIM5->SR      = (0x0000);                    // Clear all status (and interrupt) bits
    TIM5->DIER    = (0x0000);                    // Turn off the timer (update) interrupt
    TIM5->CR2     = (0x0000);                    // Ensure CR2 is at default settings
    TIM5->CR1     = (0x0000) | (TIM_CR1_CEN   |  // Turn on the timer
                                TIM_CR1_URS   |  // Only overflow/underflow generates an update IRQ
                                TIM_CR1_OPM   |  // One pulse mode, disable after count hits zero
                                TIM_CR1_ARPE);   // Buffer the ARPE
  }
  
  // Wait until timer hits the ARR value or the pending energy expenditure value reaches zero
  if (pDelay->tDelay > 0)
    while ((!(TIM5->SR & TIM_SR_UIF)) && (false == Time_accumulateEnergy(50)));
  else
    while (false == Time_accumulateEnergy(50));
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
void HAL_SYSTICK_Callback(void)
{
  Time_decrementSoftTimers();
}
