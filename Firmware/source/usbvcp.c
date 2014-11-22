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

// Define the API for the USB Core interface
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

/*************************************************************************************************\
* FUNCTION    VCP_DeInit
* DESCRIPTION Called immediately after deinitialization of the USB controller
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
static uint16 VCP_DeInit(void)
{
  return USBD_OK;
}

/*************************************************************************************************\
* FUNCTION    VCP_Ctrl
* DESCRIPTION Manages CDC class requests
* PARAMETERS  Cmd: Command code
*             Buf: Buffer containing the command data (request parameters)
*             Len: Number of bytes to respond with
* RETURNS     retVal: Result of the operation (USBD_OK in all cases)
* NOTES       None
\*************************************************************************************************/
static uint16 VCP_Ctrl(uint32 Cmd, uint8* Buf, uint32 Len)
{ 
  switch (Cmd)
  {
    case SEND_ENCAPSULATED_COMMAND: /* Not  needed for this driver */ break;
    case GET_ENCAPSULATED_RESPONSE: /* Not  needed for this driver */ break;
    case SET_COMM_FEATURE: /* Not  needed for this driver */ break;
    case GET_COMM_FEATURE: /* Not  needed for this driver */ break;
    case CLEAR_COMM_FEATURE: /* Not  needed for this driver */ break;
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
    case SET_CONTROL_LINE_STATE: /* Not  needed for this driver */ break;
    case SEND_BREAK: /* Not  needed for this driver */ break;
    default: break;
  }
  return USBD_OK;
}

/*************************************************************************************************\
* FUNCTION    VCP_DataTx
* DESCRIPTION Called immediately after deinitialization of the USB controller
* PARAMETERS  Buf: Buffer of data to be sent
*             Len: Number of data to be sent (in bytes)
* RETURNS     Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
* NOTES       This function is not called by the USB core subsystem
\*************************************************************************************************/
static uint16 VCP_DataTx (uint8* Buf, uint32 Len)
{
  return USBD_OK;
}

/*************************************************************************************************\
* FUNCTION    VCP_DataRx
* DESCRIPTION Called by the USB subsystem upon receipt of a CDC packet
* PARAMETERS  Buf: Buffer of data to be received
*             Len: Length of the incoming CDC packet
* RETURNS     Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
* NOTES       This function is not called by the USB core subsystem
\*************************************************************************************************/
static uint16 VCP_DataRx (uint8* Buf, uint32 Len)
{
  return USBD_OK;
}

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
