/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   Header for usbd_cdc_vcp.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_VCP_H
#define __USBD_CDC_VCP_H

#include "stm32f2xx.h"

#include "usbd_cdc_core.h"
#include "usbd_conf.h"
#include "types.h"
#include "usb_core.h"

/****************** USB OTG FS PHY CONFIGURATION *******************************
*  The USB OTG FS Core supports one on-chip Full Speed PHY.
*
*  The USE_EMBEDDED_PHY symbol is defined in the project compiler preprocessor
*  when FS core is used.
*******************************************************************************/
#define USE_USB_OTG_FS
#define USB_OTG_FS_CORE
#define RX_FIFO_FS_SIZE                          128
#define TX0_FIFO_FS_SIZE                          32
#define TX1_FIFO_FS_SIZE                         128
#define TX2_FIFO_FS_SIZE                          32
#define TX3_FIFO_FS_SIZE                           0

#define VBUS_SENSING_ENABLED
#define USE_DEVICE_MODE







typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
} LINE_CODING;

void USBD_initVCP(void);
boolean USBD_send(uint8 *pSrc, uint16 numBytes);

void BSP_Init(void);
void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_uDelay (const uint32_t usec);
void USB_OTG_BSP_mDelay (const uint32_t msec);
void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev);

#define DEFAULT_CONFIG                  0
#define OTHER_CONFIG                    1

#endif /* __USBD_CDC_VCP_H */
