#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "hih613x.h"
#include "i2cbb.h"
#include "spi.h"
#include "time.h"
#include "types.h"
#include "util.h"

#define FILE_ID HIH613X_C

#define HIH_I2C_ADDRESS (0x27)
#define HIH_MEASURE_CMD (HIH_I2C_ADDRESS << 1)
#define HIH_READ_CMD    (HIH_MEASURE_CMD  + 1)

#define SELECT_CHIP_HIH613X()   do { GPIOB->BSRRH |= 0x00000200; } while (0)
#define DESELECT_CHIP_HIH613X() do { GPIOB->BSRRL |= 0x00000200; } while (0)

typedef struct
{
  uint32  dontCare    : 2;
  uint32  temperature : 14;
  uint32  humidity    : 14;
  uint32  status      : 2;
} HIH613XData;

static struct
{
  HIH613XData currData;
  double currTmp;
  double currHum;
} sHIH613X;


double HIH613X_getHumidity(void)
{
  return sHIH613X.currHum;
}

double HIH613X_getTemperature(void)
{
  return sHIH613X.currTmp;
}

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
* FUNCTION    HIH613X_readTempHumidSPI
* DESCRIPTION Reads the temperature out of the sensor
* PARAMETERS  pSrc - pointer to data in EEPROM to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\*****************************************************************************/
void HIH613X_readTempHumidSPI(void)
{
  int16 hihDataBuffer[2];

  SELECT_CHIP_HIH613X();
  Util_spinWait(1000);
  SPI_read((uint8*)&hihDataBuffer, 4); // Note that SPI can only write at 800kHz
  Util_spinWait(1500);
  DESELECT_CHIP_HIH613X();
}

/**************************************************************************************************\
* FUNCTION    HIH613X_readTempHumidI2CBB
* DESCRIPTION Interfaces with the HIH613X-000 (I2C) temperature andS humidity sensor
* PARAMETERS  measure - sends a measurement command
*             read - reads the temperature and humidity from the HIH613X
*             convert - converts the temperature and humidity bit counts into degF and %RH
* RETURNS     Nothing
* NOTES       1) If both measure and read are set then the operation will be done at once with a
*                delay occurring between the measurement and read
*             2) The results are stored in the HIH613X structure
\**************************************************************************************************/
HIHStatus HIH613X_readTempHumidI2CBB(boolean measure, boolean read, boolean convert)
{
  const uint16 i2cFreqKhz = 150;
  if (measure)
    I2CBB_readData(i2cFreqKhz, HIH_MEASURE_CMD, NULL, 0); // send measure command
  if (measure && read)
    Time_delay(40); // tMeasure ~= 36.65ms, but 40ms for reliability
  if (read)
  {
    I2CBB_readData(i2cFreqKhz, HIH_READ_CMD, (uint8*)&sHIH613X.currData, sizeof(sHIH613X.currData));
    Util_swap32((uint32*)&sHIH613X.currData);
  }
  if (convert)
  {
    sHIH613X.currHum = (((double)sHIH613X.currData.humidity    / 16383.0) * 100.0);
    sHIH613X.currTmp = (((double)sHIH613X.currData.temperature / 16383.0) * 165.0) - 40.0;
    sHIH613X.currTmp = (sHIH613X.currTmp * 1.8) + 32.0;
  }
  return (HIHStatus)sHIH613X.currData.status;
}
