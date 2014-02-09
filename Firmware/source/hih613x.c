#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "hih613x.h"

#define FILE_ID HIH613X_C

#define SELECT_CHIP_HIH613X()   do { GPIOB->BSRRH |= 0x00000200; } while (0)
#define DESELECT_CHIP_HIH613X() do { GPIOB->BSRRL |= 0x00000200; } while (0)

static struct
{
  int16 currTemperature;
  int16 currHumidity;
} sHIH613X;

/*****************************************************************************\
* FUNCTION    HIH613X_init
* DESCRIPTION Initializes the EEPROM module
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
void HIH613X_init(void)
{
  const uint16 hihPortBCtrlPins = GPIO_Pin_9; // Chip Select MOSI
  const uint16 hihPortDCtrlPins = GPIO_Pin_3; // Alarm MISO

  // Initialize the HIH613X chip select and alarm lines
  GPIO_InitTypeDef hihCtrlPortB = {hihPortBCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                     GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef hihCtrlPortD = {hihPortDCtrlPins, GPIO_Mode_IN,  GPIO_Speed_25MHz, GPIO_OType_PP,
                                                     GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };

  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &hihCtrlPortB);

  GPIO_setPortClock(GPIOD, TRUE);
  GPIO_configurePins(GPIOD, &hihCtrlPortD);

  DESELECT_CHIP_HIH613X();

  Util_fillMemory(&sHIH613X, sizeof(sHIH613X), 0x00);
}

/*****************************************************************************\
* FUNCTION    HIH613X_readTemperature
* DESCRIPTION Reads the temperature out of the sensor
* PARAMETERS  pSrc - pointer to data in EEPROM to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\*****************************************************************************/
int16 HIH613X_readTemperature(void)
{
  int16 hihDataBuffer[2];

  SELECT_CHIP_HIH613X();
  Util_spinWait(1000);
  SPI_read((uint8*)&hihDataBuffer, 4); // Note that SPI can only write at 800kHz
  Util_spinWait(1500);
  DESELECT_CHIP_HIH613X();

  return hihDataBuffer[0];
}

/*****************************************************************************\
* FUNCTION    HIH613X_test
* DESCRIPTION Executes reads and writes to EEPROM to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
void HIH613X_test(void)
{
  int16 currTemp;

  Analog_setDomain(MCU_DOMAIN,    FALSE);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);   // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);   // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE);  // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);   // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE);  // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.65); // Set domain voltage to nominal (3.25V)
  Time_delay(1000); // Wait 1000ms for domains to settle

  // basic read test
  while(1)
  {
    currTemp = HIH613X_readTemperature();
    Time_delay(100); // Wait 1000ms for domains to settle
  }
}
