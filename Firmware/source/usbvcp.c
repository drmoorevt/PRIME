#include "gpio.h"
#include "misc.h"
#include "time.h"
#include "usbvcp.h"

#include "usb_core.h"
#include "usb_regs.h"
#include "usb_dcd_int.h"

#include "usbd_req.h"
#include "usbd_usr.h"
#include "usbd_cdc_core.h"
#include "usbd_desc.h"

#define FILE_ID USBVCP_C

static struct
{
  USB_OTG_CORE_HANDLE      dev;
  USB_OTG_CORE_ID_TypeDef  coreID;
  USBD_DEVICE             *pDevice;
  USBD_Class_cb_TypeDef   *class_cb;
  USBD_Usr_cb_TypeDef     *usr_cb;
} sUSBD;

USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};

typedef struct
{
  uint32 bitrate;
  uint8  format;
  uint8  paritytype;
  uint8  datatype;
} LINE_CODING;

LINE_CODING linecoding =
  {
    115200 * 8, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };

/* These are external variables imported from CDC core to be used for IN  transfer management. */
extern uint8  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32 APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */

/* Private function prototypes -----------------------------------------------*/
static uint16 VCP_Init     (void);
static uint16 VCP_DeInit   (void);
static uint16 VCP_Ctrl     (uint32 Cmd, uint8* Buf, uint32 Len);
static uint16 VCP_DataTx   (uint8* Buf, uint32 Len);
static uint16 VCP_DataRx   (uint8* Buf, uint32 Len);

CDC_IF_Prop_TypeDef VCP_fops = 
{
  VCP_Init,
  VCP_DeInit,
  VCP_Ctrl,
  VCP_DataTx,
  VCP_DataRx
};

/*************************************************************************************************\
* FUNCTION    VCP_init
* DESCRIPTION Called immediately after configuration of the USB controller
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
static uint16 VCP_Init(void)
{
  return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16 VCP_DeInit(void)
{
  return USBD_OK;
}


/**
  * @brief  VCP_Ctrl
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16 VCP_Ctrl (uint32 Cmd, uint8* Buf, uint32 Len)
{ 
  switch (Cmd)
  {
  case SEND_ENCAPSULATED_COMMAND:
    /* Not  needed for this driver */
    break;

  case GET_ENCAPSULATED_RESPONSE:
    /* Not  needed for this driver */
    break;

  case SET_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case GET_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case CLEAR_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case SET_LINE_CODING:
    linecoding.bitrate = (uint32)(Buf[0] | (Buf[1] << 8) | (Buf[2] << 16) | (Buf[3] << 24));
    linecoding.format = Buf[4];
    linecoding.paritytype = Buf[5];
    linecoding.datatype = Buf[6];
    break;

  case GET_LINE_CODING:
    Buf[0] = (uint8)(linecoding.bitrate);
    Buf[1] = (uint8)(linecoding.bitrate >> 8);
    Buf[2] = (uint8)(linecoding.bitrate >> 16);
    Buf[3] = (uint8)(linecoding.bitrate >> 24);
    Buf[4] = linecoding.format;
    Buf[5] = linecoding.paritytype;
    Buf[6] = linecoding.datatype; 
    break;

  case SET_CONTROL_LINE_STATE:
    /* Not  needed for this driver */
    break;

  case SEND_BREAK:
    /* Not  needed for this driver */
    break;    
    
  default:
    break;
  }

  return USBD_OK;
}

/**
  * @brief  VCP_DataTx
  *         CDC received data to be send over USB IN endpoint are managed in 
  *         this function.
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
  */
static uint16 VCP_DataTx (uint8* Buf, uint32 Len)
{
  return USBD_OK;
}

/**
  * @brief  VCP_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         until exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
  */
static uint16 VCP_DataRx (uint8* Buf, uint32 Len)
{
  return USBD_OK;
}

/********************************USBD_CDC_STUFF*****************************************/

#define USBD_VID                        0x0483
#define USBD_PID                        0x5740
#define USBD_LANGID_STRING              0x409
#define USBD_MANUFACTURER_STRING        "Lab304"
#define USBD_PRODUCT_HS_STRING          "STM32 Virtual ComPort in HS mode"
#define USBD_SERIALNUMBER_HS_STRING     "00000000050B"
#define USBD_PRODUCT_FS_STRING          "PEGMA Virtual ComPort in FS Mode"
#define USBD_SERIALNUMBER_FS_STRING     "00000000050C"
#define USBD_CONFIGURATION_HS_STRING    "VCP Config"
#define USBD_INTERFACE_HS_STRING        "VCP Interface"
#define USBD_CONFIGURATION_FS_STRING    "VCP Config"
#define USBD_INTERFACE_FS_STRING        "VCP Interface"

// Prototypes for the USB Device Description
USBD_DEVICE USR_desc =
{
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor, 
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
  
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8 USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_CFG_MAX_NUM            /*bNumConfigurations*/
  } ; /* USB_DeviceDescriptor */


/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8 USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8 USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
     USB_SIZ_STRING_LANGID,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};

/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_DeviceDescriptor( uint8 speed , uint16 *length)
{
  *length = sizeof(USBD_DeviceDesc);
  return USBD_DeviceDesc;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *USBD_USR_LangIDStrDescriptor( uint8 speed , uint16 *length)
{
  *length =  sizeof(USBD_LangIDDesc);  
  return USBD_LangIDDesc;
}


/**
* @brief  USBD_USR_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_ProductStrDescriptor( uint8 speed , uint16 *length)
{
  if(speed == 0)
  {   
    USBD_GetString (USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_ManufacturerStrDescriptor( uint8 speed , uint16 *length)
{
  USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_SerialStrDescriptor( uint8 speed , uint16 *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {    
    USBD_GetString (USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_ConfigStrDescriptor( uint8 speed , uint16 *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {  
    USBD_GetString (USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
  }
  return USBD_StrDesc;  
}


/**
* @brief  USBD_USR_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8 *  USBD_USR_InterfaceStrDescriptor( uint8 speed , uint16 *length)
{
  if(speed == 0)
  {
    USBD_GetString (USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;  
}

void USBD_USR_Init(void) { }
void USBD_USR_DeviceReset(uint8 speed ){ }
void USBD_USR_DeviceConfigured (void) { }
void USBD_USR_DeviceSuspended(void) { }
void USBD_USR_DeviceResumed(void) { }
void USBD_USR_DeviceConnected (void) { }
void USBD_USR_DeviceDisconnected(void) { }

/**************************************************************************************************\
* FUNCTION    USBVCP_init
* DESCRIPTION Initializes the USB Virtual Com Port
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
void USBVCP_init(void)
{
  USBD_Init(&sUSBD.dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
}

/**************************************************************************************************\
* FUNCTION    USBVCP_send
* DESCRIPTION Sends the specified data out the Virtual Com Port
* PARAMETERS  pSrc: The source buffer to send
*             numBytes: The number of bytes to send
* RETURNS     TRUE if the bytes are queued to be sent out the buffer, FALSE otherwise
\**************************************************************************************************/
boolean USBVCP_send(uint8 *pSrc, uint32 numBytes)
{
  uint32 i;
  for (i = 0; i < numBytes; i++)
  {
    APP_Rx_Buffer[APP_Rx_ptr_in++] = pSrc[i];

    /* To avoid buffer overflow */
    if(APP_Rx_ptr_in == APP_RX_DATA_SIZE)
      APP_Rx_ptr_in = 0;
  }
  return TRUE;
}

void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler(&sUSBD.dev);
}
