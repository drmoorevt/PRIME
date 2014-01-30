#include "stm32f2xx.h"
#include "adc.h"
#include "dac.h"
#include "analog.h"
#include "eeprom.h"
#include "gpio.h"
#include "serialflash.h"
#include "spi.h"
#include "tests.h"
#include "util.h"
#include "uart.h"

int main(void)
{
  //GPIO_init();  // YOU NEED TO WRITE THE EEPROM/SPI INIT FUNCTIONS SO I DONT HAVE TO USE THIS DEFAULT ONE!********************

  Time_init();
  ADC_init();
  DAC_init();
  Analog_init();
  UART_init();
  EEPROM_init();
  //SerialFlash_init();
  SPI_init();
  Tests_init();

  SerialFlash_test();
  
  while(1)
  {
    Tests_run();
  }
}
