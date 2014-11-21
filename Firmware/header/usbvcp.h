/**************************************************************************************************\
  * @file    usbd_cdc_vcp.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   Header for usbd_cdc_vcp.c file.
\**************************************************************************************************/

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

void USBVCP_init(void);
boolean USBVCP_send(uint8 *pSrc, uint16 numBytes);
uint32  USBVCP_stopReceive(void);
boolean USBVCP_openPort(void);
boolean USBVCP_closePort(void);
boolean USBVCP_sendData(uint8 *pSrc, uint16 numBytes);
boolean USBVCP_receiveData(uint32 numBytes, uint32 timeout, boolean interChar);

void BSP_Init(void);
void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_uDelay (const uint32_t usec);
void USB_OTG_BSP_mDelay (const uint32_t msec);
void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev);

#define DEFAULT_CONFIG                  0
#define OTHER_CONFIG                    1

#endif /* __USBD_CDC_VCP_H */
