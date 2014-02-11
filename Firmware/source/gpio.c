#include "stm32f2xx.h"
#include "gpio.h"

#define FILE_ID GPIO_C

/**************************************************************************************************\
* FUNCTION    GPIO_init
* DESCRIPTION Initializes the GPIO ports to their default configurations
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
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

/**************************************************************************************************\
* FUNCTION    GPIO_setPortClock
* DESCRIPTION Enables or disables clocks to the specified port
* PARAMETERS  GPIO_InitStruct: Pointer to the buffer this function will be initializing
*             pin: The pin to set
*             portNum: The port
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void GPIO_structInitUART(GPIO_InitTypeDef* GPIO_InitStruct, uint16 pin, uint8 portNum)
{
  /* Reset GPIO init structure parameters values */
  GPIO_InitStruct->GPIO_Pin      = pin;
  GPIO_InitStruct->GPIO_Mode     = GPIO_Mode_AF;
  GPIO_InitStruct->GPIO_Speed    = GPIO_Speed_2MHz;
  GPIO_InitStruct->GPIO_OType    = GPIO_OType_PP;
  GPIO_InitStruct->GPIO_PuPd     = GPIO_PuPd_NOPULL;
  GPIO_InitStruct->GPIO_AltFunc  = (portNum < 3) ? GPIO_AF_UART1_3 : GPIO_AF_UART_4_6;
}

/**************************************************************************************************\
* FUNCTION    GPIO_setPortClock
* DESCRIPTION Enables or disables clocks to the specified port
* PARAMETERS  GPIOx: The port to enable or disable
*             clockState: The clocks will be enabled if clockState is TRUE, disabled otherwise
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean GPIO_setPortClock(GPIO_TypeDef* GPIOx, boolean clockState)
{
  uint32 clockMask;
  if (GPIOx == GPIOA)
    clockMask = RCC_AHB1ENR_GPIOAEN;
  else if (GPIOx == GPIOB)
    clockMask = RCC_AHB1ENR_GPIOBEN;
  else if (GPIOx == GPIOC)
    clockMask = RCC_AHB1ENR_GPIOCEN;
  else if (GPIOx == GPIOD)
    clockMask = RCC_AHB1ENR_GPIODEN;
  else if (GPIOx == GPIOE)
    clockMask = RCC_AHB1ENR_GPIOEEN;
  else
    return ERROR;

  if (clockState)
    SET_BIT(RCC->AHB1ENR, clockMask);
  else
    CLEAR_BIT(RCC->AHB1ENR, clockMask);
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    GPIO_configurePins
* DESCRIPTION Initializes the GPIO ports to their default configurations
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean GPIO_configurePins(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct)
{
  uint32_t pinpos = 0x00, pos = 0x00 , currentpin = 0x00;

  /*-- GPIO Mode Configuration --*/
  for (pinpos = 0x00; pinpos < 0x10; pinpos++)
  {
    pos = ((uint32_t)0x01) << pinpos;
    /* Get the port pins position */
    currentpin = (GPIO_InitStruct->GPIO_Pin) & pos;

    if (currentpin == pos)
    {
      GPIOx->MODER  &= ~(GPIO_MODER_MODER0 << (pinpos * 2));
      GPIOx->MODER |= (((uint32_t)GPIO_InitStruct->GPIO_Mode) << (pinpos * 2));

      if ((GPIO_InitStruct->GPIO_Mode == GPIO_Mode_OUT) || (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_AF))
      {
        /* Speed mode configuration */
        GPIOx->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pinpos * 2));
        GPIOx->OSPEEDR |= ((uint32_t)(GPIO_InitStruct->GPIO_Speed) << (pinpos * 2));

        /* Output mode configuration*/
        GPIOx->OTYPER  &= ~((GPIO_OTYPER_OT_0) << ((uint16_t)pinpos)) ;
        GPIOx->OTYPER |= (uint16_t)(((uint16_t)GPIO_InitStruct->GPIO_OType) << ((uint16_t)pinpos));
      }

      /* Pull-up Pull down resistor configuration*/
      GPIOx->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << ((uint16_t)pinpos * 2));
      GPIOx->PUPDR |= (((uint32_t)GPIO_InitStruct->GPIO_PuPd) << (pinpos * 2));

      /* Alternate function configuration */
      if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_AF)
      {
        if (pinpos < 8)
        {
          GPIOx->AFR[0] &= ~(0x0000000F << (pinpos*4));
          GPIOx->AFR[0] |= GPIO_InitStruct->GPIO_AltFunc << (pinpos*4);
        }
        else
        {
          GPIOx->AFR[1] &= ~(0x0000000F << ((pinpos-8)*4));
          GPIOx->AFR[1] |= GPIO_InitStruct->GPIO_AltFunc << ((pinpos-8)*4);
        }
      }
    }
  }
  return SUCCESS;
}

void GPIO_setDirection(GPIO_TypeDef *GPIOx, uint16 pinNumber, GPIOMode_TypeDef outMode)
{
  GPIOx->MODER &= ~(GPIO_MODER_MODER0 << (pinNumber * 2));
  GPIOx->MODER |= (((uint32_t)outMode) << (pinNumber * 2));
}
