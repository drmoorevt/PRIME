#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "time.h"
#include "types.h"
#include "usbd.h"
#include "util.h"

#define FILE_ID USBD_C

// Pin Definitions
#define USB_LED_ID   (GPIO_Pin_10) // PortA USBLED on schematic, functionally the ID pin as well
#define USBD_DATANEG (GPIO_Pin_11) // PortA
#define USBD_DATAPOS (GPIO_Pin_12) // PortA

static struct
{

} sUSBD;

/**************************************************************************************************\
* FUNCTION    USBD_init
* DESCRIPTION Initializes the USBD module -- Configures it for default VCP but does not turn it on
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void USBD_init(void)
{
  Util_fillMemory(&sUSBD, sizeof(sUSBD), 0x00);
  USBD_setup(FALSE, USBD_VCP_BAUD_RATE_1152000);
}

/**************************************************************************************************\
* FUNCTION    USBD_setup
* DESCRIPTION Enables or Disables the pins required to operate the HIH613X sensor
* PARAMETERS  state: If TRUE, required pins will be output. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
\**************************************************************************************************/
boolean USBD_setup(boolean state, USBDBaudRate rate)
{

  GPIO_InitTypeDef USBDLedIdCtrl = {USB_LED_ID, GPIO_Mode_AF, GPIO_Speed_100MHz, GPIO_OType_OD,
                                    GPIO_PuPd_NOPULL, GPIO_AF_OTGFSHS};

  GPIO_InitTypeDef USBDDataCtrl = {(USBD_DATANEG | USBD_DATAPOS), GPIO_Mode_AF, GPIO_Speed_100MHz,
                                    GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_OTGFSHS};

  GPIO_configurePins(GPIOA, &USBDLedIdCtrl);
  GPIO_configurePins(GPIOA, &USBDDataCtrl);
  GPIO_setPortClock(GPIOA, TRUE);

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

  if (state)
    NVIC_EnableIRQ(OTG_HS_IRQn);
  else
    NVIC_DisableIRQ(OTG_HS_IRQn);

  return TRUE;
}
