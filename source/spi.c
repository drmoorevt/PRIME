#include "stm32f2xx.h"
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
  RCC->APB2RSTR |=  RCC_APB1RSTR_SPI2RST;
  RCC->APB2RSTR &= ~RCC_APB1RSTR_SPI2RST;
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; // Enable SPI peripheral clocks
  GPIOB->MODER  = ((GPIOB->MODER & 0x03FFFFFF) | 0xA8000000); //  Alternate function SPI2
  // Using 2MHz for baud rate until we start modulating voltage 30MHz / 2Mhz = 15
//  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR);
  SPI2->CR1 |= (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0);
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



