#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "eeprom.h"
#include "gpio.h"
#include "tests.h"
#include "util.h"
#include "uart.h"

int main(void)
{
  //GPIO_init();  // YOU NEED TO WRITE THE EEPROM/SPI INIT FUNCTIONS SO I DONT HAVE TO USE THIS DEFAULT ONE!********************

  Time_init();
  ADC_init();
  UART_init();
  Analog_init();
  Tests_init();
  EEPROM_init();

  Tests_run();
  while(1); // Should never get here!
}
