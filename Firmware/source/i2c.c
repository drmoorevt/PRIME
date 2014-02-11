#include "stm32f2xx.h"
#include "gpio.h"
#include "i2c.h"

#define FILE_ID I2C_C

#define I2C_TX(x)    do { I2C2->DR = x; while(!(I2C2->SR1 & I2C_SR1_TXE)); } while(0)

/*****************************************************************************\
* FUNCTION     I2C_init
* DESCRIPTION  Initializes I2C support
* PARAMETERS   none
* RETURNS      nothing
\*****************************************************************************/
void I2C_init(void)
{
  uint16 tempReg = (I2C2->CR2 & (~I2C_CR2_FREQ));
  const uint16 I2CPinsPortB = (GPIO_Pin_13 | GPIO_Pin_15);

  // Initialize all potential ADC pins to analog configuration
  GPIO_InitTypeDef i2cCtrlPortB = {I2CPinsPortB, GPIO_Mode_AF, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_I2C1_3 };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &i2cCtrlPortB);

  RCC->APB2RSTR |=  (RCC_APB1RSTR_I2C2RST);  // Reset the I2C peripheral
  RCC->APB2RSTR &=  (~RCC_APB1RSTR_I2C2RST); // Release the I2C peripheral from reset

  RCC->APB1ENR  |= (RCC_APB1ENR_I2C2EN); // Enable I2C peripheral clocks

  I2C2->CR2 = tempReg | (uint16)30;

  I2C2->TRISE = 31;

  I2C2->CCR = 150; // 30MHz/200kHz

  I2C2->CR1 = (I2C_CR1_PE);
  
  I2C_read(NULL, 0);
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
void I2C_write(const uint8 *pBytes, uint16 numBytes)
{
  uint16 dummy = I2C2->DR; // clear the DR

  if (numBytes == 0)
    return;
  
  while(numBytes--)
    I2C_TX(*pBytes++);
  
	while(I2C2->SR2 & I2C_SR2_BUSY); //wait for I2C to be complete
}

/*****************************************************************************\
* FUNCTION     I2C_read
* DESCRIPTION  Reads the specified number of bytes from the I2C and places them
*              into the buffer pointed to by pBytes.
* PARAMETERS   pBytes - buffer in which to the bytes read
*              numBytes - the number of bytes to read
* RETURNS      nothing
\*****************************************************************************/
void I2C_read(uint8 *pBytes, uint16 numBytes)
{
  uint16 dummy = I2C2->DR; // clear the DR
  
  if (numBytes == 0)
    return;

  while(numBytes--)
  {
		//while (!(I2C2->SR1 & I2C_SR1_RXNE));
    *pBytes++ = I2C2->DR;
  }

	while(I2C2->SR2 & I2C_SR2_BUSY); //wait for I2C to be complete
}



