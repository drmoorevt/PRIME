#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "bluetooth.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "hih613x.h"
#include "i2cbb.h"
#include "sdcard.h"
#include "serialflash.h"
#include "spi.h"
#include "sram.h"
#include "tests.h"
#include "usbvcp.h"
#include "util.h"
#include "uart.h"
#include "zigbee.h"

// Depends on Analog_init
void Main_init(void)
{
  while (RCC->CR != 0x0F038F83) // Ensure that the PLL is functioning
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

boolean Main_powerOnSelfTest(void)
{
  return SRAM_test();
}

void Main_run(void)
{
  while (TRUE)
  {
    Tests_run();
  }
}

void Main_error(void)
{
  while (TRUE);
}

int main(void)
{
  Time_init();
  ADC_init();
  DAC_init();
  Analog_init();
  Main_init();
  UART_init();
  Bluetooth_init();
  HIH613X_init();
  EEPROM_init();
  SerialFlash_init();
  SDCard_init();
  SPI_init();
  SRAM_init();
  I2CBB_init();
  USBVCP_init();
  Tests_init();
  ZigBee_init();
  
  if (Main_powerOnSelfTest())
    Main_run();
  else
    Main_error();
}
