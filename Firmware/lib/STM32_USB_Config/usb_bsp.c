/**
  ******************************************************************************
  * @file    usb_bsp.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file is responsible to offer board support package and is
  *          configurable by user.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "misc.h"
#include "time.h"
#include "usb_bsp.h"

/** @addtogroup USB_OTG_DRIVER
* @{
*/

/** @defgroup USB_BSP
  * @brief This file is responsible to offer board support package
  * @{
  */ 

/** @defgroup USB_BSP_Private_Defines
  * @{
  */ 

// Pin Definitions
#define USB_VBUS      (GPIO_Pin_9) // PortA 
#define USB_LED_ID   (GPIO_Pin_10) // PortA USBLED on schematic, functionally the ID pin as well
#define USBD_DATANEG (GPIO_Pin_11) // PortA
#define USBD_DATAPOS (GPIO_Pin_12) // PortA

/**
  * @}
  */ 


/** @defgroup USB_BSP_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 





/** @defgroup USB_BSP_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_BSP_Private_Variables
  * @{
  */ 

/**
  * @}
  */ 

/** @defgroup USBH_BSP_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USB_BSP_Private_Functions
  * @{
  */ 


/**
  * @brief  USB_OTG_BSP_Init
  *         Initilizes BSP configurations
  * @param  None
  * @retval None
  */

void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev)
{
  GPIO_InitTypeDef USBDVBusCtrl = {USB_VBUS, GPIO_Mode_IN, GPIO_Speed_100MHz, 
                                   GPIO_OType_OD, GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM};
  
  GPIO_InitTypeDef USBDLedIdCtrl = {USB_LED_ID, GPIO_Mode_AF, GPIO_Speed_100MHz, 
                                    GPIO_OType_OD, GPIO_PuPd_UP, GPIO_AF_OTGFSHS};

  GPIO_InitTypeDef USBDDataCtrl = {(USBD_DATANEG | USBD_DATAPOS), GPIO_Mode_AF, GPIO_Speed_100MHz,
                                    GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_OTGFSHS};

  NVIC_DisableIRQ(OTG_FS_IRQn);

  RCC->AHB2RSTR |= RCC_AHB2RSTR_OTGFSRST;
  RCC->AHB2RSTR &= (~RCC_AHB2RSTR_OTGFSRST);
  Time_delay(1000);

  GPIO_setPortClock(GPIOA, TRUE);
  
  GPIO_configurePins(GPIOA, &USBDVBusCtrl);
  GPIO_configurePins(GPIOA, &USBDLedIdCtrl);
  GPIO_configurePins(GPIOA, &USBDDataCtrl);

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
}
/**
  * @brief  USB_OTG_BSP_EnableInterrupt
  *         Enabele USB Global interrupt
  * @param  None
  * @retval None
  */
void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev)
{
  NVIC_InitTypeDef NVIC_InitStructure; 
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;  
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);  
}

/**
  * @brief  BSP_Drive_VBUS
  *         Drives the Vbus signal through IO
  * @param  speed : Full, Low 
  * @param  state : VBUS states
  * @retval None
  */

void USB_OTG_BSP_DriveVBUS(uint32_t speed, uint8_t state)
{

}

/**
  * @brief  USB_OTG_BSP_ConfigVBUS
  *         Configures the IO for the Vbus and OverCurrent
  * @param  Speed : Full, Low 
  * @retval None
  */

void  USB_OTG_BSP_ConfigVBUS(uint32_t speed)
{

}

/**
  * @brief  USB_OTG_BSP_TimeInit
  *         Initialises delay unit Systick timer /Timer2
  * @param  None
  * @retval None
  */
void USB_OTG_BSP_TimeInit ( void )
{

}

/**
  * @brief  USB_OTG_BSP_uDelay
  *         This function provides delay time in micro sec
  * @param  usec : Value of delay required in micro sec
  * @retval None
  */
void USB_OTG_BSP_uDelay (const uint32_t usec)
{

  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if ( ++count > utime )
    {
      return ;
    }
  }
  while (1); 
  
}


/**
  * @brief  USB_OTG_BSP_mDelay
  *          This function provides delay time in milli sec
  * @param  msec : Value of delay required in milli sec
  * @retval None
  */
void USB_OTG_BSP_mDelay (const uint32_t msec)
{

    USB_OTG_BSP_uDelay(msec * 1000);    

}


/**
  * @brief  USB_OTG_BSP_TimerIRQ
  *         Time base IRQ
  * @param  None
  * @retval None
  */

void USB_OTG_BSP_TimerIRQ (void)
{

} 

/**
* @}
*/ 

/**
* @}
*/ 

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
