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
  /*
  GPIO_init();
  
  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  TRUE);  // Enable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, FALSE); // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  */
  
//  Tests_test0();  
//  Tests_test1();
//  Tests_test2();  
//  Tests_test3();
//  Tests_test4();
//  Tests_test5();
//  Tests_test6();
  Tests_test7();
}
