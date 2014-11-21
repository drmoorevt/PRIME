#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
//#include "bluetooth.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "hih613x.h"
#include "i2cbb.h"
//#include "sdcard.h"
//#include "serialflash.h"
#include "spi.h"
#include "tests.h"
#include "usbd_cdc_vcp.h"
#include "util.h"
#include "uart.h"
//#include "zigbee.h"

//static __INLINE void __enable_irq()               { __ASM volatile ("cpsie i"); }
//static __INLINE void __disable_irq()              { __ASM volatile ("cpsid i"); }

#include <stdio.h>
#include <string.h>
char danString[32];

void Main_init(void)
{
  while (RCC->CR != 0x0F038F83)
    SystemInit();
  
  Analog_setDomain(MCU_DOMAIN,    FALSE, 3.3);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN,  TRUE, 3.3);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,      TRUE, 3.3);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,   TRUE, 3.3);  // Enable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE, 3.3);  // Disable sram domain
  Analog_setDomain(SPI_DOMAIN,    FALSE, 3.3);  // Disable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE, 3.3);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE, 3.3);  // Disable relay domain
}

int main(void)
{
  volatile uint32 i;
  
  Time_init();
  ADC_init();
  DAC_init();
  Analog_init();
  Main_init();	// depends on Analog_init
  UART_init();
//  Bluetooth_init();
  HIH613X_init();
  EEPROM_init();
//  SerialFlash_init();
//  SDCard_init();
  SPI_init();
  I2CBB_init();
//  Tests_init();
//  USBD_initVCP();
//  ZigBee_init();
  
  USBD_initVCP();
  Time_delay(1000000);
  
  while(1)
  {
    //for (i=0; i<12000000; i++);
    sprintf(danString, "Hello Dan! %016lu\r\n", (uint32)SysTick->VAL);
    USBD_send((uint8*)&danString, 32);
    //__disable_irq();
    //Time_delay(250000);
    //__enable_irq();
    //Time_delay(1000000);
    
    //Tests_run();
  }
}
