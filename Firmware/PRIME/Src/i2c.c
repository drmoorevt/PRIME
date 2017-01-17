#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "time.h"

#define FILE_ID I2C_C

I2C_HandleTypeDef *spI2C;

/*****************************************************************************\
* FUNCTION     I2C_init
* DESCRIPTION  Initializes I2C support
* PARAMETERS   none
* RETURNS      nothing
\*****************************************************************************/
void I2C_init(I2C_HandleTypeDef *pI2C)
{
  spI2C = pI2C;
  I2C_setup(false, I2C_CLOCK_RATE_400000);
}

/*****************************************************************************\
* FUNCTION     I2C_init
* DESCRIPTION  Initializes I2C support
* PARAMETERS   none
* RETURNS      nothing
\*****************************************************************************/
bool I2C_setup(boolean state, I2CClockRate rate)
{
  HAL_I2C_DeInit(spI2C);
  if (state)
  {
    switch (rate)
    {
      case I2C_CLOCK_RATE_400000: spI2C->Init.ClockSpeed = 400000; break;
      case I2C_CLOCK_RATE_200000: spI2C->Init.ClockSpeed = 200000; break;
      case I2C_CLOCK_RATE_100000: spI2C->Init.ClockSpeed = 100000; break;
      case I2C_CLOCK_RATE_050000: spI2C->Init.ClockSpeed = 50000;  break;
      case I2C_CLOCK_RATE_025000: spI2C->Init.ClockSpeed = 25000;  break;
      case I2C_CLOCK_RATE_012500: spI2C->Init.ClockSpeed = 12500;  break;
      default: return FALSE;
    }
    HAL_I2C_Init(spI2C);
  }
  return true;
}

/*****************************************************************************\
* FUNCTION     I2C_Send7bitAddress
* DESCRIPTION  Writes numBytes beginning at pBytes to the I2C.
* PARAMETERS   pBytes -- the bytes to write
*              numBytes -- the number of bytes to write
* RETURNS      nothing
\*****************************************************************************/
void I2C_send7bitAddress(uint8 address, uint8 direction)
{
  /* Test on the direction to set/reset the read/write bit */
  if (direction != 0)
    address |= I2C_OAR1_ADD0; /* Set the address bit0 for read */
  else
    address &= (uint8) ~ ((uint8)I2C_OAR1_ADD0); /* Reset the address bit0 for write */

  I2C2->DR = address; /* Send the address */
}

/*****************************************************************************\
* FUNCTION     I2C_write
* DESCRIPTION  Writes numBytes beginning at pBytes to the I2C.
* PARAMETERS   pBytes -- the bytes to write
*              numBytes -- the number of bytes to write
* RETURNS      nothing
\*****************************************************************************/
bool I2C_write(uint8_t devAddr, uint8 *pBytes, uint16 numBytes)
{
  bool retVal = (HAL_OK == HAL_I2C_Master_Transmit(spI2C, devAddr << 1, pBytes, numBytes, 100));
  if (!retVal)
    return false;
  // Wait for the last byte and stop to be transmitted
  uint32_t timeout = 1000;
  while ((timeout-- > 0) && (spI2C->Instance->SR2 & I2C_SR2_BUSY))
    Time_delay(1);
  return (timeout > 0);
}

/*****************************************************************************\
* FUNCTION     I2C_read
* DESCRIPTION  Reads the specified number of bytes from the I2C and places them
*              into the buffer pointed to by pBytes.
* PARAMETERS   pBytes - buffer in which to the bytes read
*              numBytes - the number of bytes to read
* RETURNS      nothing
\*****************************************************************************/
bool I2C_read(uint8_t devAddr, uint8 *pBytes, uint16 numBytes)
{
  bool retVal = (HAL_OK == HAL_I2C_Master_Receive(spI2C, devAddr << 1, pBytes, numBytes, 100));
  return retVal;
  /*
  if (!retVal)
    return false;
  // Wait for the last byte and stop to be transmitted
  uint32_t timeout = 1000;
  while ((timeout-- > 0) && (spI2C->Instance->SR2 & I2C_SR2_BUSY))
    Time_delay(1);
  return (timeout > 0);
  */
}

/*****************************************************************************\
* FUNCTION     I2C_write
* DESCRIPTION  Writes numBytes beginning at pBytes to the I2C.
* PARAMETERS   pBytes -- the bytes to write
*              numBytes -- the number of bytes to write
* RETURNS      nothing
\*****************************************************************************/
bool I2C_memWrite(uint8_t devAddr, uint16_t memAddr, uint8 *pBytes, uint16 numBytes)
{
  HAL_I2C_Mem_Write(spI2C, devAddr << 1, memAddr, I2C_MEMADD_SIZE_8BIT, pBytes, numBytes, 100);
  // Wait for the last byte and stop to be transmitted
  uint32_t timeout = 1000;
  while ((timeout-- > 0) && (spI2C->Instance->SR2 & I2C_SR2_BUSY))
    Time_delay(1);
  return (timeout > 0);
}

/*****************************************************************************\
* FUNCTION     I2C_read
* DESCRIPTION  Reads the specified number of bytes from the I2C and places them
*              into the buffer pointed to by pBytes.
* PARAMETERS   pBytes - buffer in which to the bytes read
*              numBytes - the number of bytes to read
* RETURNS      nothing
\*****************************************************************************/
bool I2C_memRead(uint8_t devAddr, uint16_t memAddr, uint8 *pBytes, uint16 numBytes)
{
  HAL_I2C_Mem_Read(spI2C, devAddr << 1, memAddr, I2C_MEMADD_SIZE_8BIT, pBytes, numBytes, 100);
  // Wait for the last byte and stop to be transmitted
  uint32_t timeout = 1000;
  while ((timeout-- > 0) && (spI2C->Instance->SR2 & I2C_SR2_BUSY))
    Time_delay(1);
  return (timeout > 0);
}
