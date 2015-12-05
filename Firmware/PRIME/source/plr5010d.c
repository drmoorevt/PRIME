#include "plr5010d.h"
#include "stm32f4xx_hal.h"

static struct
{
  SPI_HandleTypeDef spiHandle;
  DMA_HandleTypeDef txHandle;
  DMA_HandleTypeDef rxHandle;
  uint8 txBuffer[32];
  uint8 rxBuffer[32];
  
} sPLR5010D;

/**************************************************************************************************\
* FUNCTION    DMA2_Stream3_IRQHandler
* DESCRIPTION Catches interrupts from the DMA stream and dispatches them to their handler
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       DMA2, Stream3 is configured for DMA via SPI_5 (DMA Channel 2)
\**************************************************************************************************/
void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(sPLR5010D.spiHandle.hdmarx);
  return;
}

/**************************************************************************************************\
* FUNCTION    DMA2_Stream4_IRQHandler
* DESCRIPTION Catches interrupts from the DMA stream and dispatches them to their handler
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       DMA2, Stream4 is configured for DMA via SPI_5 (DMA Channel 2)
\**************************************************************************************************/
void DMA2_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(sPLR5010D.spiHandle.hdmatx);
  return;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_init
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean PLR5010D_init(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  SPIx_SCK_GPIO_CLK_ENABLE();   /* Enable GPIO TX/RX/CLK/CS */
  SPIx_MISO_GPIO_CLK_ENABLE();
  SPIx_MOSI_GPIO_CLK_ENABLE();
  PLRx_CS_CLK_ENABLE();
  SPIx_CLK_ENABLE();     /* Enable SPI3 clock */
  DMAx_CLK_ENABLE();     /* Enable DMA2 clock */
  
  /*##-2- Configure peripheral GPIO ##########################################*/  
  /* SPI SCK GPIO pin configuration  */
  GPIO_InitStruct.Pin       = SPIx_SCK_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
  GPIO_InitStruct.Alternate = SPIx_SCK_AF;
  
  HAL_GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);
    
  /* SPI MISO GPIO pin configuration  */
  GPIO_InitStruct.Pin = SPIx_MISO_PIN;
  GPIO_InitStruct.Alternate = SPIx_MISO_AF;
  
  HAL_GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStruct);
  
  /* SPI MOSI GPIO pin configuration  */
  GPIO_InitStruct.Pin = SPIx_MOSI_PIN;
  GPIO_InitStruct.Alternate = SPIx_MOSI_AF;
    
  HAL_GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStruct);

  /* CS GPIO pin configuration  */
  GPIO_InitStruct.Pin  = PLRx_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    
  HAL_GPIO_Init(PLRx_CS_GPIO_PORT, &GPIO_InitStruct);
    
  /*##-3- Configure the DMA streams ##########################################*/
  /* Configure the DMA handler for Transmission process */
  sPLR5010D.txHandle.Instance                 = SPIx_TX_DMA_STREAM;
  
  sPLR5010D.txHandle.Init.Channel             = SPIx_TX_DMA_CHANNEL;
  sPLR5010D.txHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  sPLR5010D.txHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  sPLR5010D.txHandle.Init.MemInc              = DMA_MINC_ENABLE;
  sPLR5010D.txHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  sPLR5010D.txHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  sPLR5010D.txHandle.Init.Mode                = DMA_NORMAL;
  sPLR5010D.txHandle.Init.Priority            = DMA_PRIORITY_LOW;
  sPLR5010D.txHandle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;         
  sPLR5010D.txHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  sPLR5010D.txHandle.Init.MemBurst            = DMA_MBURST_INC4;
  sPLR5010D.txHandle.Init.PeriphBurst         = DMA_PBURST_INC4;
  
  HAL_DMA_Init(&sPLR5010D.txHandle);   
  
  /* Associate the initialized DMA handle to the the SPI handle */
  __HAL_LINKDMA(&sPLR5010D.spiHandle, hdmatx, sPLR5010D.txHandle);
    
  /* Configure the DMA handler for Transmission process */
  sPLR5010D.rxHandle.Instance                 = SPIx_RX_DMA_STREAM;
  
  sPLR5010D.rxHandle.Init.Channel             = SPIx_RX_DMA_CHANNEL;
  sPLR5010D.rxHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  sPLR5010D.rxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  sPLR5010D.rxHandle.Init.MemInc              = DMA_MINC_ENABLE;
  sPLR5010D.rxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  sPLR5010D.rxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  sPLR5010D.rxHandle.Init.Mode                = DMA_NORMAL;
  sPLR5010D.rxHandle.Init.Priority            = DMA_PRIORITY_HIGH;
  sPLR5010D.rxHandle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;         
  sPLR5010D.rxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  sPLR5010D.rxHandle.Init.MemBurst            = DMA_MBURST_INC4;
  sPLR5010D.rxHandle.Init.PeriphBurst         = DMA_PBURST_INC4; 

  HAL_DMA_Init(&sPLR5010D.rxHandle);
    
  /* Associate the initialized DMA handle to the the SPI handle */
  __HAL_LINKDMA(&sPLR5010D.spiHandle, hdmarx, sPLR5010D.rxHandle);
    
  /*##-4- Configure the NVIC for DMA #########################################*/ 
  /* NVIC configuration for DMA transfer complete interrupt (SPI3_TX) */
  HAL_NVIC_SetPriority(SPIx_DMA_TX_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(SPIx_DMA_TX_IRQn);
    
  /* NVIC configuration for DMA transfer complete interrupt (SPI3_RX) */
  HAL_NVIC_SetPriority(SPIx_DMA_RX_IRQn, 0, 0);   
  HAL_NVIC_EnableIRQ(SPIx_DMA_RX_IRQn);
  
  sPLR5010D.spiHandle.Instance               = SPIx;
  sPLR5010D.spiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  sPLR5010D.spiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
  sPLR5010D.spiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
  sPLR5010D.spiHandle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
  sPLR5010D.spiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  sPLR5010D.spiHandle.Init.CRCPolynomial     = 7;
  sPLR5010D.spiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
  sPLR5010D.spiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  sPLR5010D.spiHandle.Init.NSS               = SPI_NSS_SOFT;
  sPLR5010D.spiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
  sPLR5010D.spiHandle.Init.Mode              = SPI_MODE_MASTER;

  if (HAL_SPI_Init(&sPLR5010D.spiHandle) != HAL_OK)
    return FALSE;
  
  while (HAL_SPI_GetState(&sPLR5010D.spiHandle) != HAL_SPI_STATE_READY);
  
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    PLR5010D_setVoltage
* DESCRIPTION 
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean PLR5010D_setVoltage(uint16 outBitsA, uint16 outBitsB)
{
  /******** Write DAC A ********/
  sPLR5010D.txBuffer[0] = 0x00;
  sPLR5010D.txBuffer[1] = outBitsA >> 8;
  sPLR5010D.txBuffer[2] = outBitsA >> 0;
  
  HAL_GPIO_WritePin(PLRx_CS_GPIO_PORT, PLRx_CS_PIN, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit_DMA(&sPLR5010D.spiHandle, sPLR5010D.txBuffer, 3) != HAL_OK)
    return FALSE;
  while (HAL_SPI_GetState(&sPLR5010D.spiHandle) != HAL_SPI_STATE_READY);
  HAL_GPIO_WritePin(PLRx_CS_GPIO_PORT, PLRx_CS_PIN, GPIO_PIN_SET);
  
  /******** Write DAC B ********/
  sPLR5010D.txBuffer[0] = 0x01;
  sPLR5010D.txBuffer[1] = outBitsB >> 8;
  sPLR5010D.txBuffer[2] = outBitsB >> 0;
  
  HAL_GPIO_WritePin(PLRx_CS_GPIO_PORT, PLRx_CS_PIN, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit_DMA(&sPLR5010D.spiHandle, sPLR5010D.txBuffer, 3) != HAL_OK)
    return FALSE;
  while (HAL_SPI_GetState(&sPLR5010D.spiHandle) != HAL_SPI_STATE_READY);
  HAL_GPIO_WritePin(PLRx_CS_GPIO_PORT, PLRx_CS_PIN, GPIO_PIN_SET);  
  
  return TRUE;
}
