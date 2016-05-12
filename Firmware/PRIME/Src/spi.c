#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "spi.h"

#define FILE_ID SPI_C

#define SPI_MSCK_PIN (GPIO_Pin_13)
#define SPI_MISO_PIN (GPIO_Pin_14)
#define SPI_MOSI_PIN (GPIO_Pin_15)

#define SPI_TX(x)    do { SPI2->DR = x; while(!(SPI2->SR&SPI_SR_TXE)); } while(0)

/**************************************************************************************************\
* FUNCTION     SPI_init
* DESCRIPTION  Initializes SPI support
* PARAMETERS   none
* RETURNS      nothing
\**************************************************************************************************/
void SPI_init(void)
{
  SPI_setup(FALSE, SPI_CLOCK_RATE_1625000);
}

/**************************************************************************************************\
* FUNCTION     SPI_setup
* DESCRIPTION
* PARAMETERS   none
* RETURNS      TRUE
\**************************************************************************************************/
boolean SPI_setup(boolean state, SPIClockRate rate)
{
//  GPIO_InitTypeDef spiPortB  = {(SPI_MSCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN), GPIO_Mode_AF,
//                                GPIO_Speed_25MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM};

  GPIO_InitTypeDef spiPortB  = {(SPI_MSCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN), GPIO_Mode_AF,
                                GPIO_Speed_100MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM};
  spiPortB.GPIO_Mode    = (state == TRUE) ? GPIO_Mode_AF   : GPIO_Mode_IN;
  spiPortB.GPIO_AltFunc = (state == TRUE) ? GPIO_AF_SPI1_2 : GPIO_AF_SYSTEM;

  RCC->APB2RSTR |=  (RCC_APB1RSTR_SPI2RST);  // Reset the SPI peripheral
  RCC->APB2RSTR &=  (~RCC_APB1RSTR_SPI2RST); // Release the SPI peripheral from reset
  RCC->APB1ENR  |= (RCC_APB1ENR_SPI2EN);     // Enable SPI peripheral clocks
  SPI2->CR1  = (SPI_CR1_SPE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (rate << 3));
  SPI2->CR2 = (0x00000000);

  GPIO_configurePins(GPIOB, &spiPortB);
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION     SPI_write
* DESCRIPTION  Writes numBytes beginning at pBytes to the SPI.
* PARAMETERS   pBytes -- the bytes to write
*              numBytes -- the number of bytes to write
* RETURNS      nothing
\**************************************************************************************************/
void SPI_write(const uint8 *pBytes, uint32 numBytes)
{
  uint16 dummy = SPI2->DR; // clear the DR
  if (numBytes == 0)
    return;
  
  while(numBytes--)
    SPI_TX(*pBytes++);
  
  while(SPI2->SR & SPI_SR_BSY); // Wait for tx to complete
}

/*****************************************************************************\
* FUNCTION     SPI_read
* DESCRIPTION  Reads the specified number of bytes from the SPI and places them
*              into the buffer pointed to by pBytes.
* PARAMETERS   pBytes - buffer in which to the bytes read
*              numBytes - the number of bytes to read
* RETURNS      nothing
\*****************************************************************************/
void SPI_read(uint8 *pBytes, uint32 numBytes)
{
  uint16 dummy = SPI2->DR; // clear the DR
  
  if (numBytes == 0)
    return;

  while(numBytes--)
  {
    SPI_TX(0xFF);
		while (!(SPI2->SR & SPI_SR_RXNE));
    *pBytes++ = SPI2->DR;   // read the incoming byte
  }

	while(SPI2->SR & SPI_SR_BSY); // Wait for tx to complete
}



