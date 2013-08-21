#include "stm32f2xx.h"
#include "gpio.h"

/*****************************************************************************\
* FUNCTION    GPIO_init
* DESCRIPTION Initializes the GPIO ports to their default configurations
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
void GPIO_init(void)
{ 
  // Port A Configuration
  RCC->AHB1ENR  |= 0x00000001; // Enable port A interface clocks
	GPIOA->MODER   = 0xAA817FFC; // FFFFFIIOOAAAAAAI -- input on busvout for now, 5v toleration as an input
  GPIOA->OTYPER  = 0x00000000; // All push-pull
  GPIOA->OSPEEDR = 0x00000000; // 2MHz output speed
  GPIOA->PUPDR   = 0x64000000; // No pullup or pulldown
  GPIOA->AFR[1]  = 0x000AA000;
  GPIOA->AFR[0]  = 0x00000000;
  
  // Port B Configuration
  RCC->AHB1ENR  |= 0x00000002; // Enable port B interface clocks
//	GPIOB->MODER   = 0xA8A5A695; // FFFIFFOOFFOFFOOO // note that I am keeping PB4 as JTAG here, per errata
	GPIOB->MODER   = 0x00A5A695; // FFFIFFOOFFOFFOOO // note that I am keeping PB4 as JTAG here, per errata
//  GPIOB->OTYPER  = 0x0000E324; // SPI open collector
  GPIOB->OTYPER  = 0x00000304; // SPI not open collector
  GPIOB->OSPEEDR = 0x540008C0; // 2MHz output speed
  GPIOB->PUPDR   = 0x00000100; // No pullup or pulldown
  GPIOB->AFR[1]  = 0x55507700;
  GPIOB->AFR[0]  = 0x77000000;
  
  // Port C Configuration
  RCC->AHB1ENR  |= 0x00000004; // Enable port C interface clocks
	GPIOC->MODER   = 0x02A5A555; // IIIF FFOO FFOO OOOO
  GPIOC->OTYPER  = 0x00000000; // all push-pull
  GPIOC->OSPEEDR = 0x00000000; // 2MHz output speed
  GPIOC->PUPDR   = 0x00090AAA; // 485-DE:D,~RE:H; DOM-RST:D,S3:D,S0:D,S2:D,S1:D,EN:D
  GPIOC->AFR[1]  = 0x00088800; // Enable Alternate Function for UART5TX
  GPIOC->AFR[0]  = 0x88000000;
  
  // Port D Configuration
  RCC->AHB1ENR  |= 0x00000008; // Enable port D interface clocks
	GPIOD->MODER   = 0x00001020; // IIII IIII IOII IFII
  GPIOD->OTYPER  = 0x00000000; // Only XBE_RST output
  GPIOD->OSPEEDR = 0x00000000; // 2MHz output speed
  GPIOD->PUPDR   = 0x00000000; // No pullup or pulldown
  GPIOD->AFR[1]  = 0x00000000;
  GPIOD->AFR[0]  = 0x00000800;  // Enable Alternate Function for UART5RX
  
  // Port E Configuration
  RCC->AHB1ENR  |= 0x00000010; //Enable port C interface clocks
	GPIOE->MODER   = 0x00001550; // IIII IIII IOOO OOII
  GPIOE->OTYPER  = 0x00000078; // relays are open collector
  GPIOE->OSPEEDR = 0x00000000; // 2MHz output speed
  GPIOE->PUPDR   = 0x00000010; // DOMLEN:H
  GPIOE->AFR[1]  = 0x00000000;
  GPIOE->AFR[0]  = 0x00000000;
}

#define FILE_ID GPIO_C
