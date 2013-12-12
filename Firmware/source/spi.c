#include "stm32f2xx.h"
#include "gpio.h"
#include "spi.h"

#define FILE_ID SPI_C

#define SPI_TX(x)    do { SPI2->DR = x; while(!(SPI2->SR&SPI_SR_TXE)); } while(0)

/*****************************************************************************\
* FUNCTION     SPI_init
* DESCRIPTION  Initializes the SPI support.
* PARAMETERS   none
* RETURNS      nothing
\*****************************************************************************/
void SPI_init(void)
{
  const uint16 spiPinsPortB = (GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);

  // Initialize all potential ADC pins to analog configuration
  GPIO_InitTypeDef spiCtrlPortB = {spiPinsPortB, GPIO_Mode_AF, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SPI1_2 };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &spiCtrlPortB);

  RCC->APB2RSTR |=  (RCC_APB1RSTR_SPI2RST);  // Reset the SPI peripheral
  RCC->APB2RSTR &=  (~RCC_APB1RSTR_SPI2RST); // Release the SPI peripheral from reset

  RCC->APB1ENR  |= (RCC_APB1ENR_SPI2EN); // Enable SPI peripheral clocks

  // Using 2MHz for baud rate until we start modulating voltage 30MHz / 2Mhz = 15
//  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR);

  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_2);

//  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_2);

  //  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI);
  SPI2->CR2 = (0x00000000);
}

/*****************************************************************************\
* FUNCTION     SPI_write
* DESCRIPTION  Writes numBytes beginning at pBytes to the SPI.
* PARAMETERS   pBytes -- the bytes to write
*              numBytes -- the number of bytes to write
* RETURNS      nothing
\*****************************************************************************/
void SPI_write(const uint8 *pBytes, uint16 numBytes)
{
  uint16 dummy = SPI2->DR; // clear the DR
  // pBytes may point to RAM or ROM
  if (numBytes == 0)
    return;

  
  //UCB1IFG = 0x00; //clear rx/tx flags
  
  
  while(numBytes--)
    SPI_TX(*pBytes++);
  
	while(SPI2->SR & SPI_SR_BSY); //wait for spi to be complete
//  while(P5IN & BIT5); //UCBUSY is cleared when the data is done, need clk to settle to idle too
}

/*****************************************************************************\
* FUNCTION     SPI_read
* DESCRIPTION  Reads the specified number of bytes from the SPI and places them
*              into the buffer pointed to by pBytes.
* PARAMETERS   pBytes - buffer in which to the bytes read
*              numBytes - the number of bytes to read
* RETURNS      nothing
\*****************************************************************************/
void SPI_read(uint8 *pBytes, uint16 numBytes)
{
  uint16 dummy = SPI2->DR; // clear the DR
  
  if (numBytes == 0)
  {
    return; // no assert, as this code is called in assert
  }

//  UCB1IFG = 0x00; //clear rx/tx flags

  while(numBytes--)
  {
    SPI_TX(0xFF);
		while (!(SPI2->SR & SPI_SR_RXNE));
    *pBytes++ = SPI2->DR;   // read the incoming byte
  }
	while(SPI2->SR & SPI_SR_BSY); //wait for spi to be complete
//  while(P5IN & BIT5); //UCBUSY is cleared when the data is done, need clk to settle to idle too
}



