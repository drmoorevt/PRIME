#include "extusb.h"

#define FT232H_DATA_AVAILABLE  (0x01)
#define FT232H_SPACE_AVAILABLE (0x02)
#define FT232H_SUSPEND         (0x04)
#define FT232H_CONFIGURED      (0x08)
#define FT232H_BUFFER_SIZE     (1024)
static char helloString[FT232H_BUFFER_SIZE];

uint8_t volatile * const pDataPipe = (uint8_t *)0x64000000;
uint8_t volatile * const pStatPipe = (uint8_t *)0x64000001;

/**************************************************************************************************\
* FUNCTION    ExtUSB_init
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool ExtUSB_init(void)
{
  return true;
}

/**************************************************************************************************\
* FUNCTION    ExtUSB_tx
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool ExtUSB_tx(uint8_t *pSrc, uint32_t len)
{
  while(len--)
  {
    while(!(*pStatPipe & FT232H_SPACE_AVAILABLE));  // Wait for available buffer space
    *(pDataPipe) = *(uint8_t *)pSrc++;
  }
  return true;
}

/**************************************************************************************************\
* FUNCTION    ExtUSB_rx
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool ExtUSB_rx(uint8_t *pDst, uint32_t len)
{
  return true;
}

/**************************************************************************************************\
* FUNCTION    ExtUSB_testUSB
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool ExtUSB_testUSB(void)
{
  uint32_t len = sprintf((char *)helloString, "Hello world!");
  ExtUSB_tx((uint8_t *)helloString, len);
  return true;
}
