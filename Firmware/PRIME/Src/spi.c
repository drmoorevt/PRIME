#include "stm32f4xx_hal.h"
#include "spi.h"

#define FILE_ID SPI_C

static SPI_HandleTypeDef *spSPI;

/**************************************************************************************************\
* FUNCTION     SPI_init
* DESCRIPTION  Initializes SPI support
* PARAMETERS   none
* RETURNS      nothing
\**************************************************************************************************/
void SPI_init(SPI_HandleTypeDef *pSPI)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_SPI5_CLK_ENABLE();
  
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  
  // Configure the SPI5 CS pins
  GPIO_InitStruct.Pin = PLR0_CS_Pin|PLR1_CS_Pin|PLR2_CS_Pin;
  //GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = SD_CS_Pin|CSX_Pin;
  //GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = OF_CS_Pin;
  //GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = AF_CS_Pin|EE_CS_Pin;
  //GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  
  HAL_GPIO_WritePin(PLR0_CS_GPIO_Port, PLR0_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PLR1_CS_GPIO_Port, PLR1_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PLR2_CS_GPIO_Port, PLR2_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port,   SD_CS_Pin,   GPIO_PIN_SET);
  HAL_GPIO_WritePin(OF_CS_GPIO_Port,   OF_CS_Pin,   GPIO_PIN_SET);
  HAL_GPIO_WritePin(AF_CS_GPIO_Port,   AF_CS_Pin,   GPIO_PIN_SET);
  HAL_GPIO_WritePin(EE_CS_GPIO_Port,   EE_CS_Pin,   GPIO_PIN_SET);
  HAL_GPIO_WritePin(CSX_GPIO_Port,     CSX_Pin,     GPIO_PIN_SET);
  
  spSPI = pSPI;
  SPI_setup(FALSE, SPI_CLOCK_RATE_MAX, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
}

/**************************************************************************************************\
* FUNCTION     SPI_setup
* DESCRIPTION
* PARAMETERS   none
* RETURNS      TRUE
\**************************************************************************************************/
boolean SPI_setup(boolean state, SPIRate rate, uint32_t phase, uint32_t pol, SPITIMode mode)
{
  HAL_SPI_DeInit(spSPI);  // dont deinit every time?
  if (state)
  {
    uint32_t prescalar;
    switch (rate)
    {
      case SPI_CLOCK_RATE_45000000: prescalar = SPI_BAUDRATEPRESCALER_2;   break;
      case SPI_CLOCK_RATE_22500000: prescalar = SPI_BAUDRATEPRESCALER_4;   break;
      case SPI_CLOCK_RATE_11250000: prescalar = SPI_BAUDRATEPRESCALER_8;   break;
      case SPI_CLOCK_RATE_05625000: prescalar = SPI_BAUDRATEPRESCALER_16;  break;
      case SPI_CLOCK_RATE_02812500: prescalar = SPI_BAUDRATEPRESCALER_32;  break;
      case SPI_CLOCK_RATE_01406250: prescalar = SPI_BAUDRATEPRESCALER_64;  break;
      case SPI_CLOCK_RATE_00703125: prescalar = SPI_BAUDRATEPRESCALER_128; break;
      default: return FALSE;
    }
    
    spSPI->Init.Direction      = SPI_DIRECTION_2LINES;
    spSPI->Init.CLKPhase       = SPI_PHASE_1EDGE;
    spSPI->Init.CLKPolarity    = SPI_POLARITY_LOW;
    spSPI->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    spSPI->Init.CRCPolynomial  = 7;
    spSPI->Init.DataSize       = SPI_DATASIZE_8BIT;
    spSPI->Init.FirstBit       = SPI_FIRSTBIT_MSB;
    spSPI->Init.NSS            = SPI_NSS_SOFT;
    spSPI->Init.TIMode         = SPI_TIMODE_DISABLED;
    spSPI->Init.Mode           = SPI_MODE_MASTER;
    
    spSPI->Init.BaudRatePrescaler = prescalar;
    spSPI->Init.CLKPhase = (phase == SPI_PHASE_2EDGE) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    spSPI->Init.CLKPolarity = (pol == SPI_POLARITY_HIGH) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    spSPI->Init.TIMode = (mode == SPI_MODE_TI) ? SPI_TIMODE_ENABLE : SPI_TIMODE_DISABLE;
    HAL_SPI_Init(spSPI);
  }
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
  if (numBytes == 0)
    return;
  
  HAL_SPI_Transmit(spSPI, (uint8_t *)pBytes, numBytes, 100);
  while(spSPI->State != HAL_SPI_STATE_READY); // Wait for tx to complete
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
  if (numBytes == 0)
    return;
  
  memset(pBytes, 0xFF, numBytes);
  
  HAL_SPI_Receive(spSPI, (uint8_t *)pBytes, numBytes, 100);
  while(spSPI->State != HAL_SPI_STATE_READY); // Wait for tx to complete
}



