/*
 * analog.h
 *
 *  Created on: Mar 8, 2015
 *      Author: drmoore
 */

#ifndef HEADER_ANALOG_H_
#define HEADER_ANALOG_H_

#include "types.h"

#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC3_CLK_ENABLE()
#define DMAx_CLK_ENABLE()               __HAL_RCC_DMA2_CLK_ENABLE()
#define ADCx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC_RELEASE_RESET()

/* Definition for ADCx Channel Pin */
#define ADCx_CHANNEL_PIN                (GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5)
#define ADCx_CHANNEL_GPIO_PORT          GPIOC

/* Definition for ADCx's NVIC */
#define ADCx_DMA_IRQn                   DMA2_Stream0_IRQn
#define ADCx_DMA_IRQHandler             DMA2_Stream0_IRQHandler


boolean Analog_testAnalogBandwidth(void);

#endif /* HEADER_ANALOG_H_ */
