#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "sdcard.h"

#define FILE_ID SDCARD_C

#define WRITEPAGESIZE_SF ((uint16)256)
#define SF_NUM_RETRIES 3
#define ADDRBYTES_SF 3

#define SELECT_CHIP_SF()    do { GPIOB->BSRRH |= 0x00000010; } while (0)
#define DESELECT_CHIP_SF()  do { GPIOB->BSRRL |= 0x00000010; } while (0)

//Serial flash global protection register constants
#define SF_UNPROTECT_GLOBAL_gc  0x00
#define SF_PROTECT_GLOBAL_gc    0x3c

#define OP_WRITE_ENABLE     0x06
#define OP_WRITE_MEMORY     0x02
#define OP_READ_MEMORY      0x03
#define OP_WRITE_STATUS     0x01
#define OP_READ_STATUS      0x05

//Serial flash status register
#define SF_SPRL_bm          0x80
#define SF_EPE_bm           0x20
#define SF_WPn_bm           0x10
#define SF_SWP_bm           0x0c
#define SF_WEL_bm           0x02
#define SF_BSY_bm           0x01

#define SF_SWP_SOME_gc      0x04

static struct
{
  SDCardState state;
} sSDCard;

/*****************************************************************************\
* FUNCTION    SDCard_init
* DESCRIPTION Initializes the SDCard module
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
void SDCard_init(void)
{
  const uint16 sfCtrlPins = (GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_8);

  // Initialize the chip select and hold lines
  /*
  GPIO_InitTypeDef eeCtrlPortB = {eeCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_OD,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  */
  GPIO_InitTypeDef eeCtrlPortB = {sfCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &eeCtrlPortB);
  DESELECT_CHIP_SF();

  Util_fillMemory((uint8*)&sSDCard, sizeof(sSDCard), 0x00);
  sSDCard.state = SD_CARD_IDLE;
}

/*****************************************************************************\
* FUNCTION    SDCard_getState
* DESCRIPTION Returns the internal state of SDCard
* PARAMETERS  None
* RETURNS     The SDCardState
\*****************************************************************************/
SDCardState SDCard_getState(void)
{
  return sSDCard.state;
}

/*****************************************************************************\
* FUNCTION    SDCard_waitStateFlash
* DESCRIPTION Wait for the status register to transition to specified
*             value or timeout
* PARAMETERS  state - expected state
*             stateMask = bit mask to consider
*             timeout - timeout in ticks
* RETURNS     status byte for serial flash
\*****************************************************************************/
static uint8 SDCard_waitStateFlash(uint8 state, uint8 stateMask, uint16 timeout)
{
  uint8 status;

  Time_startTimer(TIMER_SERIAL_MEM, timeout);
  status = OP_READ_STATUS;
  SELECT_CHIP_SF();
  SPI_write(&status,1);
  do
  {
    // This can take a while, pet the watchdog
//    WDTCTL = WDT_ARST_1000;
    SPI_read(&status,1);
  } while(((status & stateMask) != state) && Time_getTimerValue(TIMER_SERIAL_MEM));
  DESELECT_CHIP_SF();

  return status;
}

/*****************************************************************************\
* FUNCTION    SDCard_readFlash
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in serial flash to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\*****************************************************************************/
void SDCard_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 chipOffset;
  uint8 writeBuf[ADDRBYTES_SF + 1];

  SPI_init();

  chipOffset = (uint32)pSrc;

  writeBuf[0] = OP_READ_MEMORY;

  writeBuf[1] = (uint8)(chipOffset >> 16);
  writeBuf[2] = (uint8)(chipOffset >> 8);
  writeBuf[3] = (uint8)chipOffset;

  SELECT_CHIP_SF();
  // read from SDCard
  SPI_write(writeBuf,ADDRBYTES_SF + 1);
  SPI_read(pDest,length);

  DESELECT_CHIP_SF();
}

/*****************************************************************************\
* FUNCTION    SDCard_writeFlash
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SDCard
*             length - number of bytes to write
* RETURNS     true if the write suceeds
\*****************************************************************************/
boolean SDCard_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 chipOffset;
  uint16 numBytes;
  uint8 writeBuf[ADDRBYTES_SF + 1];
  uint8 retries;
  boolean writeFailed = FALSE;
  uint8 writeResult;

  SPI_init();

  //UNPROTECT_CHIP_SF();
  while (!writeFailed && length > 0)
  {
    // for SF, write must not go past a 256 byte boundary
    numBytes = WRITEPAGESIZE_SF - ((uint32)pDest & (WRITEPAGESIZE_SF-1));
    if (length < numBytes)
      numBytes = length;

    chipOffset = (uint32)pDest;
    retries = SF_NUM_RETRIES;
    writeBuf[1] = (uint8)(chipOffset >> 16);
    writeBuf[2] = (uint8)(chipOffset >> 8);
    writeBuf[3] = (uint8)chipOffset;

    do
    {
      // enable FLASH write
      SELECT_CHIP_SF();
      writeBuf[0] = OP_WRITE_ENABLE;
      SPI_write(writeBuf,1);
      DESELECT_CHIP_SF();

      // write to FLASH
      writeBuf[0] = OP_WRITE_MEMORY;
      SELECT_CHIP_SF();
      SPI_write(writeBuf,ADDRBYTES_SF + 1);
      SPI_write(pSrc,numBytes);
      DESELECT_CHIP_SF();

      // wait for write to complete, set a timeout of 2 ticks which will be between 8 and 16 msec
      // so we won't watchdog if the SDCard gets stuck.
      writeResult = SDCard_waitStateFlash(0, SF_BSY_bm, 2);

      //Analyze the result
      if (writeResult & SF_EPE_bm)
      {
        writeFailed = TRUE;
      }
      else if((writeResult & SF_SWP_bm) == SF_SWP_bm)
      {

        writeFailed = TRUE; //all sectors are protected
        break; // retries won't help
      }
      else if((writeResult & SF_SWP_bm) == SF_SWP_SOME_gc)
      {
        /*
        //some sectors are protected, is ours one of them?
        writeBuf[0] = OP_READ_PROTECTION;
        SELECT_CHIP_SF();
        SPI_write(writeBuf, ADDRBYTES_SF + 1);
        SPI_read(writeBuf, 1);
        DESELECT_CHIP_SF();

        if(writeBuf[0] != 0)  //Yes, protected 64kB memory sector is written
        {                     //retry won't help
          writeFailed = TRUE;
          break;
        }
        */
      }
    }while(writeFailed && --retries != 0);

    pSrc += numBytes;   // update source pointer
    pDest +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  //PROTECT_CHIP_SF();
  // If we were never able to write to FLASH, we need to flag an error
  //if (writeFailed)
  //  Health_setStatusFlag(MODULE_ERROR, SDCard_ERROR);

  return (boolean)!writeFailed;
}

/*****************************************************************************\
* FUNCTION    SDCard_protectFlash
* DESCRIPTION protect or unprotect the SF chip (global)
* PARAMETERS  bProtect - boolean to protect = TRUE, unprotect = FALSE
* RETURNS     status byte for serial flash
\*****************************************************************************/
uint8 SDCard_protectFlash(boolean bProtect)
{
  uint8 buf[2];

  SPI_init();

  //read status
  buf[0] = OP_READ_STATUS;
  SELECT_CHIP_SF();
  SPI_write(buf, 1);
  SPI_read(buf, 1);
  DESELECT_CHIP_SF();

  //make we sure we not already protected /unprotected
  if ( ( bProtect && ((buf[0] & 0x0c) != 0x0c)) ||
       (!bProtect && ((buf[0] & 0x0c) != 0x00)) )
  {
    //UNPROTECT_CHIP_SF();
    if((buf[0] & SF_SPRL_bm) != 0) //SPRL bit set?
    {
      //unprotect the protection register first

      //enable chip write
      buf[0] = OP_WRITE_ENABLE;
      SELECT_CHIP_SF();
      SPI_write(buf, 1);
      DESELECT_CHIP_SF();

      //write the SPRL bit = 0
      buf[0] = OP_WRITE_STATUS;
      buf[1] = 0;   //SF_SPRL_bm = 0
      SELECT_CHIP_SF();
      SPI_write(buf, 2);
      DESELECT_CHIP_SF();
    }

    //enable chip write
    buf[0] = OP_WRITE_ENABLE;
    SELECT_CHIP_SF();
    SPI_write(buf, 1);
    DESELECT_CHIP_SF();

    //write global protection register
    buf[0] = OP_WRITE_STATUS;
    if(bProtect)
    {
      buf[1] = SF_PROTECT_GLOBAL_gc;
    }
    else
    {
      buf[1] = SF_UNPROTECT_GLOBAL_gc;
    }
    SELECT_CHIP_SF();
    SPI_write(buf, 2);
    DESELECT_CHIP_SF();

    if(bProtect)
      buf[1] = SF_SWP_bm;
    else
      buf[1] = 0;

    buf[0] = SDCard_waitStateFlash(0, (SF_SWP_bm |SF_BSY_bm), 2);
    //UNPROTECT_CHIP_SF();
  }
  return buf[0];
}

/*****************************************************************************\
* FUNCTION    SDCard_eraseFlash
* DESCRIPTION Erase a block of flash
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to erase
* RETURNS     status byte for serial flash
\*****************************************************************************/
uint8 SDCard_eraseFlash(uint8 *pDest, SDCardBlockSize blockSize)
{
  uint8 numBytes;
  uint8 writeBuf[ADDRBYTES_SF + 1];


  SELECT_CHIP_SF();
   // enable FLASH write
  writeBuf[0] = OP_WRITE_ENABLE;
  SPI_write(writeBuf,1);
  DESELECT_CHIP_SF();

  writeBuf[0] = (uint8)blockSize;
  if(blockSize != SD_CARD_CHIP)
  {
    writeBuf[1] = (uint8)((uint32)pDest >> 16);
    writeBuf[2] = (uint8)((uint32)pDest >> 8);
    writeBuf[3] = (uint8)((uint32)pDest >> 0);
    numBytes = ADDRBYTES_SF + 1;
  }
  else
  {
    numBytes = 1;
  }

  SELECT_CHIP_SF();
  SPI_write(writeBuf,numBytes);
  DESELECT_CHIP_SF();

  // wait for Erase to complete, max 3500ms
  return SDCard_waitStateFlash(0, SF_BSY_bm, 3500);
}

/*****************************************************************************\
* FUNCTION    SDCard_test
* DESCRIPTION Executes reads and writes to SDCard to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
void SDCard_test(void)
{
  uint8 buffer[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  uint8 test[sizeof(buffer)];
  
  Analog_setDomain(MCU_DOMAIN,    FALSE); // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);  // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);  // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE); // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE); // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);  // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE); // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE); // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6); // Set domain voltage to nominal
  Time_delay(1000); // Wait 1000ms for domains to settle

  while(1)
  {
    // basic read test
    SDCard_readFlash((uint8*)0,test,sizeof(test));
    SDCard_writeFlash(buffer,(uint8*)0,sizeof(buffer));
    SDCard_readFlash((uint8*)0,test,sizeof(test));

    // boundary test
    SDCard_writeFlash(buffer,(uint8*)(8),sizeof(buffer));
    SDCard_readFlash((uint8*)(8),test,sizeof(test));
  }
}
