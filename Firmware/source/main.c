#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "hih613x.h"
#include "i2c.h"
#include "sdcard.h"
#include "serialflash.h"
#include "spi.h"
#include "tests.h"
#include "util.h"
#include "uart.h"

int main(void)
{
  Time_init();
  ADC_init();
  DAC_init();
  Analog_init();
  UART_init();
  HIH613X_init();
  EEPROM_init();
  SerialFlash_init();
  SDCard_init();
  SPI_init();
  Tests_init();
  
  while(1)
  {
    Tests_run();
  }
}
