#include "stm32f2xx.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "eeprom.h"

#define FILE_ID EEPROM_C

#define WRITEPAGESIZE_EE ((uint16)128)
#define WRITEPAGESIZE_SF ((uint16)256)
#define EE_NUM_RETRIES 3
#define SF_NUM_RETRIES 3

#define OP_WRITE_ENABLE     0x06
#define OP_WRITE_MEMORY     0x02
#define OP_READ_MEMORY      0x03
#define OP_WRITE_STATUS     0x01
#define OP_READ_STATUS      0x05

//Serial flash global protection register constants
#define SF_UNPROTECT_GLOBAL_gc  0x00
#define SF_PROTECT_GLOBAL_gc    0x3c

#define ADDRBYTES_EE 2
#define ADDRBYTES_SF 3

//#define SELECT_CHIP_EE0()   do { GPIOB->BSRRH |= 0x00000020; Util_spinWait(200); } while (0)
//#define DESELECT_CHIP_EE0() do { Util_spinWait(200); GPIOB->BSRRL |= 0x00000020; } while (0)

#define SELECT_CHIP_EE0()   do { GPIOB->BSRRH |= 0x00000020; } while (0)
#define DESELECT_CHIP_EE0() do { GPIOB->BSRRL |= 0x00000020; } while (0)

#define SELECT_CHIP_EE1()   do { GPIOB->BSRRH |= 0x00000100; } while (0)
#define DESELECT_CHIP_EE1() do { GPIOB->BSRRL |= 0x00000100; } while (0)
#define SELECT_CHIP_SF()    do { GPIOB->BSRRH |= 0x00000010; } while (0)
#define DESELECT_CHIP_SF()  do { GPIOB->BSRRL |= 0x00000010; } while (0)

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
  EEPROMState state;
} sEEPROM;

static const uint8 sFillBuf[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void EEPROM_init(void)
{
  Util_fillMemory((uint8*)&sEEPROM, sizeof(sEEPROM), 0x00);
  sEEPROM.state = EEPROM_IDLE;
  GPIOB->BSRRL = 0x0004;
  SPI_init();
}

EEPROMState EEPROM_getState(void)
{
  return sEEPROM.state;
}

/*****************************************************************************\
* FUNCTION    EEPROM_readEE
* DESCRIPTION Reads data out of EEPROM
* PARAMETERS  pSrc - pointer to data in EEPROM to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\*****************************************************************************/
void EEPROM_readEE(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint16 chipOffset;
  uint8 writeBuf[ADDRBYTES_EE + 1];

  chipOffset = (uint16)((uint32)pSrc);

  writeBuf[0] = OP_READ_MEMORY;

  writeBuf[1] = (uint8)(chipOffset >> 8);
  writeBuf[2] = (uint8)chipOffset;

  SELECT_CHIP_EE0();
  // read from EEPROM
  SPI_write(writeBuf,ADDRBYTES_EE + 1);
  SPI_read(pDest,length);

  DESELECT_CHIP_EE0();
}

/*****************************************************************************\
* FUNCTION    EEPROM_writeEE
* DESCRIPTION Writes a buffer to EEPROM
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in EEPROM
*             length - number of bytes to write
* RETURNS     true if the write suceeds
\*****************************************************************************/
boolean EEPROM_writeEE(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint16 chipOffset, numBytes;
  uint8 writeBuf[ADDRBYTES_EE + 1], readBuf[WRITEPAGESIZE_EE];
  uint8 retries;
  boolean writeFailed = FALSE;

  while (length > 0)
  {
    // for EE, write must not go past a 16 byte boundary
    numBytes = WRITEPAGESIZE_EE - ((uint32)pDest & (WRITEPAGESIZE_EE-1));
    if (length < numBytes)
      numBytes = length;

    chipOffset = (uint16)((uint32)pDest);
    retries = EE_NUM_RETRIES;

    sEEPROM.state = EEPROM_WRITING;
    do
    {
      SELECT_CHIP_EE0();

       // enable EEPROM
      writeBuf[0] = OP_WRITE_ENABLE;
      SPI_write(writeBuf,1);

      DESELECT_CHIP_EE0();

      // write to EEPROM
      writeBuf[0] = OP_WRITE_MEMORY;

      writeBuf[2] = (uint8)(chipOffset & 0xFF);
      writeBuf[1] = (uint8)(chipOffset >> 8);

      SELECT_CHIP_EE0();
      SPI_write(writeBuf,ADDRBYTES_EE + 1);
      SPI_write(pSrc,numBytes);

      DESELECT_CHIP_EE0();

      // wait for write to complete, set a timeout of 2 ticks which will be between 8 and 16 msec
      // so we won't watchdog if the eeprom gets stuck.
      sEEPROM.state = EEPROM_WAITING;
      Time_startTimer(TIMER_SERIAL_MEM, 10);
      
      /*
      do
      {
        writeBuf[0] = OP_READ_STATUS;
        SELECT_CHIP_EE0();
        SPI_write(writeBuf,1);
        SPI_read(writeBuf,1);
        DESELECT_CHIP_EE0();
      } while((writeBuf[0] & 0x01) && Time_getTimerValue(TIMER_SERIAL_MEM));
      */
      
      while(Time_getTimerValue(TIMER_SERIAL_MEM));
      
      sEEPROM.state = EEPROM_READBACK;
      EEPROM_readEE((uint8*)chipOffset, readBuf, numBytes);

      writeFailed = (Util_compareMemory(pSrc, readBuf, (uint8)numBytes) != 0);
    }while(writeFailed && --retries != 0);
       
    pSrc += numBytes;   // update source pointer
    pDest +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  sEEPROM.state = EEPROM_IDLE;

  return (boolean)!writeFailed;
}

/*****************************************************************************\
* FUNCTION    EEPROM_zeroEE
* DESCRIPTION Zero's a section of eeprom
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to zero
* RETURNS     nothing
\*****************************************************************************/
void EEPROM_zeroEE(uint8 *pDest, uint16 length)
{
  uint8 fillLength;
  
  while (length)
  {
    fillLength = (uint8)length;

    if (length > sizeof(sFillBuf))
      fillLength = sizeof(sFillBuf);
    
    (void)EEPROM_writeEE((uint8*)sFillBuf,pDest,fillLength);
    
    length -= fillLength;
    pDest += fillLength;  
  }
}

/*****************************************************************************\
* FUNCTION    EEPROM_waitStateFlash
* DESCRIPTION Wait for the status register to transition to specified 
*             value or timeout
* PARAMETERS  state - expected state
*             stateMask = bit mask to consider
*             timeout - timeout in ticks
* RETURNS     status byte for serial flash
\*****************************************************************************/
static uint8 EEPROM_waitStateFlash(uint8 state, uint8 stateMask, uint16 timeout)
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
* FUNCTION    EEPROM_readFlash
* DESCRIPTION Reads data out of Serial flash
* PARAMETERS  pSrc - pointer to data in serial flash to read out
*             pDest - pointer to destination RAM buffer
*             length - number of bytes to read
* RETURNS     nothing
\*****************************************************************************/
void EEPROM_readFlash(uint8 *pSrc, uint8 *pDest, uint16 length)
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
  // read from EEPROM
  SPI_write(writeBuf,ADDRBYTES_SF + 1);
  SPI_read(pDest,length);

  DESELECT_CHIP_SF();
}

/*****************************************************************************\
* FUNCTION    EEPROM_writeFlash
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in EEPROM
*             length - number of bytes to write
* RETURNS     true if the write suceeds
\*****************************************************************************/
boolean EEPROM_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length)
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
      // so we won't watchdog if the eeprom gets stuck. 
      writeResult = EEPROM_waitStateFlash(0, SF_BSY_bm, 2);

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
  //  Health_setStatusFlag(MODULE_ERROR, SERIALFLASH_ERROR);
  
  return (boolean)!writeFailed;
}

/*****************************************************************************\
* FUNCTION    EEPROM_protectFlash
* DESCRIPTION protect or unprotect the SF chip (global)
* PARAMETERS  bProtect - boolean to protect = TRUE, unprotect = FALSE
* RETURNS     status byte for serial flash
\*****************************************************************************/
uint8 EEPROM_protectFlash(boolean bProtect)
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

    buf[0] = EEPROM_waitStateFlash(0, (SF_SWP_bm |SF_BSY_bm), 2);
    //UNPROTECT_CHIP_SF();
  }
  return buf[0];
}

/*****************************************************************************\
* FUNCTION    EEPROM_eraseFlash
* DESCRIPTION Erase a block of flash
* PARAMETERS  pDest - pointer to destination location
*             length - number of bytes to erase
* RETURNS     status byte for serial flash
\*****************************************************************************/
uint8 EEPROM_eraseFlash(uint8 *pDest, SFBlockSize blockSize)
{
  uint8 numBytes;
  uint8 writeBuf[ADDRBYTES_SF + 1];


  SELECT_CHIP_SF();
   // enable FLASH write
  writeBuf[0] = OP_WRITE_ENABLE;
  SPI_write(writeBuf,1);
  DESELECT_CHIP_SF();

  writeBuf[0] = (uint8)blockSize;
  if(blockSize != SF_CHIP)
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

  // wait for Erase to complete, max 3.5 sec
  return EEPROM_waitStateFlash(0, SF_BSY_bm, TIME_ONE_SECOND_IN_SUBTICKS * 10);
}

/*****************************************************************************\
* FUNCTION    EEPROM_test
* DESCRIPTION Executes reads and writes to EEPROM to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
void EEPROM_test(void)
{
  uint8 buffer[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  uint8 test[sizeof(buffer)];

  // basic read test
  EEPROM_readEE((uint8*)0,test,sizeof(test));
  EEPROM_writeEE(buffer,(uint8*)0,sizeof(buffer));
  EEPROM_readEE((uint8*)0,test,sizeof(test));

  // boundary test
  EEPROM_writeEE(buffer,(uint8*)(8),sizeof(buffer));
  EEPROM_readEE((uint8*)(8),test,sizeof(test));

  while(1);
}
