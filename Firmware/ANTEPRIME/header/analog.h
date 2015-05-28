/*
 * analog.h
 *
 *  Created on: Mar 8, 2015
 *      Author: drmoore
 */

#ifndef HEADER_ANALOG_H_
#define HEADER_ANALOG_H_

#include "types.h"

/******************* ADC Definitions ********************************************/

#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC3_CLK_ENABLE()
#define DMAx_CLK_ENABLE()               __HAL_RCC_DMA2_CLK_ENABLE()
#define ADCx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC_RELEASE_RESET()

/* Definition for ADCx Channel Pin */
#define ADCC_CHANNEL_PIN                (GPIO_PIN_2 | GPIO_PIN_3)
#define ADCC_CHANNEL_GPIO_PORT          GPIOC
#define ADCA_CHANNEL_PIN                (GPIO_PIN_7)
#define ADCA_CHANNEL_GPIO_PORT          GPIOA

/* Definition for ADCx's NVIC */
#define ADCx_DMA_IRQn                   DMA2_Stream0_IRQn
#define ADCx_DMA_IRQHandler             DMA2_Stream0_IRQHandler

/******************* DAC Definitions ********************************************/

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()

#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

/* Definition for ADCx Channel Pin */
#define DACx_CHANNEL_PIN                (GPIO_PIN_5)
#define DACx_CHANNEL_GPIO_PORT          GPIOA

/******************** Public Functions ******************************************/

boolean Analog_testAnalogBandwidth(void);
boolean Analog_testDAC(void);

#endif /* HEADER_ANALOG_H_ */
