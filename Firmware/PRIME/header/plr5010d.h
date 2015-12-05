#ifndef HEADER_PLR5010D_H_
#define HEADER_PLR5010D_H_

#include "types.h"

/* Definition for SPIx clock resources */
#define SPIx                             SPI5
#define SPIx_CLK_ENABLE()                __HAL_RCC_SPI5_CLK_ENABLE()
#define DMAx_CLK_ENABLE()                __HAL_RCC_DMA2_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOF_CLK_ENABLE()
#define SPIx_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOF_CLK_ENABLE() 
#define SPIx_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOF_CLK_ENABLE() 
#define PLRx_CS_CLK_ENABLE()             __HAL_RCC_GPIOC_CLK_ENABLE()

#define SPIx_FORCE_RESET()               __HAL_RCC_SPI5_FORCE_RESET()
#define SPIx_RELEASE_RESET()             __HAL_RCC_SPI5_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPIx_SCK_PIN                     GPIO_PIN_7
#define SPIx_SCK_GPIO_PORT               GPIOF
#define SPIx_SCK_AF                      GPIO_AF5_SPI5

#define SPIx_MISO_PIN                    GPIO_PIN_8
#define SPIx_MISO_GPIO_PORT              GPIOF
#define SPIx_MISO_AF                     GPIO_AF5_SPI5

#define SPIx_MOSI_PIN                    GPIO_PIN_9
#define SPIx_MOSI_GPIO_PORT              GPIOF
#define SPIx_MOSI_AF                     GPIO_AF5_SPI5

#define PLRx_CS_PIN                      GPIO_PIN_13
#define PLRx_CS_GPIO_PORT                GPIOC

/* Definition for SPIx's DMA */
#define SPIx_TX_DMA_CHANNEL              DMA_CHANNEL_2
#define SPIx_TX_DMA_STREAM               DMA2_Stream4
#define SPIx_RX_DMA_CHANNEL              DMA_CHANNEL_2
#define SPIx_RX_DMA_STREAM               DMA2_Stream3

/* Definition for SPIx's NVIC */
#define SPIx_DMA_TX_IRQn                 DMA2_Stream4_IRQn
#define SPIx_DMA_RX_IRQn                 DMA2_Stream3_IRQn
#define SPIx_DMA_TX_IRQHandler           DMA2_Stream4_IRQHandler
#define SPIx_DMA_RX_IRQHandler           DMA2_Stream3_IRQHandler

boolean PLR5010D_init(void);
boolean PLR5010D_setVoltage(uint16 outBitsA, uint16 outBitsB);

#endif /* HEADER_PLR5010D_H_ */
