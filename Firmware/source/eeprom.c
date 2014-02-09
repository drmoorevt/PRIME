#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "eeprom.h"

#define FILE_ID EEPROM_C

#define WRITEPAGESIZE_EE ((uint16)128)
#define EE_NUM_RETRIES 3

#define OP_WRITE_ENABLE     0x06
#define OP_WRITE_MEMORY     0x02
#define OP_READ_MEMORY      0x03
#define OP_WRITE_STATUS     0x01
#define OP_READ_STATUS      0x05

#define ADDRBYTES_EE 2

//#define SELECT_CHIP_EE0()   do { GPIOB->BSRRH |= 0x00000020; Util_spinWait(200); } while (0)
//#define DESELECT_CHIP_EE0() do { Util_spinWait(200); GPIOB->BSRRL |= 0x00000020; } while (0)

#define SELECT_CHIP_EE0()   do { GPIOB->BSRRH |= 0x00000020; } while (0)
#define DESELECT_CHIP_EE0() do { GPIOB->BSRRL |= 0x00000020; } while (0)

#define SELECT_CHIP_EE1()   do { GPIOB->BSRRH |= 0x00000100; } while (0)
#define DESELECT_CHIP_EE1() do { GPIOB->BSRRL |= 0x00000100; } while (0)

#define SELECT_EE_HOLD()    do { GPIOB->BSRRH |= 0x00000004; } while (0)
#define DESELECT_EE_HOLD()  do { GPIOB->BSRRL |= 0x00000004; } while (0)

static struct
{
  EEPROMState state;
} sEEPROM;

static const uint8 sFillBuf[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*****************************************************************************\
* FUNCTION    EEPROM_init
* DESCRIPTION Initializes the EEPROM module
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
void EEPROM_init(void)
{
  const uint16 eeCtrlPins = (GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_8);

  // Initialize the EEPROM chip select and hold lines
  /*
  GPIO_InitTypeDef eeCtrlPortB = {eeCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_OD,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  */
  GPIO_InitTypeDef eeCtrlPortB = {eeCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &eeCtrlPortB);
  DESELECT_CHIP_EE0();
  DESELECT_CHIP_EE1();
  DESELECT_EE_HOLD();

  Util_fillMemory((uint8*)&sEEPROM, sizeof(sEEPROM), 0x00);
  sEEPROM.state = EEPROM_IDLE;
}

/*****************************************************************************\
* FUNCTION    EEPROM_getState
* DESCRIPTION Returns the current state of EEPROM
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
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
* RETURNS     true if the write succeeds
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

    do
    {
      sEEPROM.state = EEPROM_WRITING;
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

      // wait for write to complete, set a timeout of 10 ticks which will be between 10-11ms
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
    } while (writeFailed && (retries-- != 0));
       
    pSrc   += numBytes;  // update source pointer
    pDest  +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  sEEPROM.state = EEPROM_IDLE;

  return (boolean)!writeFailed;
}

/*****************************************************************************\
* FUNCTION    EEPROM_writeEE
* DESCRIPTION Writes a buffer to EEPROM
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in EEPROM
*             length - number of bytes to write
* RETURNS     true if the write succeeds
\*****************************************************************************/
boolean EEPROM_writeEELowPower(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint16 chipOffset, numBytes;
  uint8 writeBuf[ADDRBYTES_EE + 1], readBuf[WRITEPAGESIZE_EE];
  uint8 retries;
  boolean writeFailed = FALSE;

  while (length > 0)
  {
    Analog_adjustDomain(EEPROM_DOMAIN, 0.0); // High voltage for high speed write
    
    // for EE, write must not go past a 16 byte boundary
    numBytes = WRITEPAGESIZE_EE - ((uint32)pDest & (WRITEPAGESIZE_EE-1));
    if (length < numBytes)
      numBytes = length;

    chipOffset = (uint16)((uint32)pDest);
    retries = EE_NUM_RETRIES;

    do
    {
      sEEPROM.state = EEPROM_WRITING;
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

      // Decreasing domain voltage for the flash erase/write cycle
//      Analog_adjustDomain(EEPROM_DOMAIN, .775); // Low voltage for low speed write
      Analog_adjustDomain(EEPROM_DOMAIN, 1.35); // Low voltage for low speed write

      // wait for write to complete, set a timeout of 10 ticks which will be between 10-11ms
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

      // Increasing domain voltage for the readback cycle
      Analog_adjustDomain(EEPROM_DOMAIN, 0.2);

      sEEPROM.state = EEPROM_READBACK;
      EEPROM_readEE((uint8*)chipOffset, readBuf, numBytes);

      writeFailed = (Util_compareMemory(pSrc, readBuf, (uint8)numBytes) != 0);
    } while (writeFailed && (retries-- != 0));

    pSrc   += numBytes;  // update source pointer
    pDest  +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  sEEPROM.state = EEPROM_IDLE;
  // Reset domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6);

  return (boolean)!writeFailed;
}

/*****************************************************************************\
* FUNCTION    EEPROM_writeEE
* DESCRIPTION Writes a buffer to EEPROM
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in EEPROM
*             length - number of bytes to write
* RETURNS     true if the write succeeds
\*****************************************************************************/
boolean EEPROM_writeEEXLP(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint16 chipOffset, numBytes;
  uint8 writeBuf[ADDRBYTES_EE + 1], readBuf[WRITEPAGESIZE_EE];
  uint8 retries;
  boolean writeFailed = FALSE;

  while (length > 0)
  {
    Analog_adjustDomain(EEPROM_DOMAIN, 1.25); // High voltage for high speed write

    // for EE, write must not go past a 16 byte boundary
    numBytes = WRITEPAGESIZE_EE - ((uint32)pDest & (WRITEPAGESIZE_EE-1));
    if (length < numBytes)
      numBytes = length;

    chipOffset = (uint16)((uint32)pDest);
    retries = EE_NUM_RETRIES;

    do
    {
      sEEPROM.state = EEPROM_WRITING;
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

      // Decreasing domain voltage for the flash erase/write cycle
//      Analog_adjustDomain(EEPROM_DOMAIN, .775); // Low voltage for low speed write
      Analog_adjustDomain(EEPROM_DOMAIN, 1.35); // Low voltage for low speed write

      // wait for write to complete, set a timeout of 10 ticks which will be between 10-11ms
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

      // Increasing domain voltage for the readback cycle
      Analog_adjustDomain(EEPROM_DOMAIN, 1.25);

      sEEPROM.state = EEPROM_READBACK;
      EEPROM_readEE((uint8*)chipOffset, readBuf, numBytes);

      writeFailed = (Util_compareMemory(pSrc, readBuf, (uint8)numBytes) != 0);
    } while (writeFailed && (retries-- != 0));

    pSrc   += numBytes;  // update source pointer
    pDest  +=  numBytes; // update destination pointer
    length -= numBytes;
  }
  sEEPROM.state = EEPROM_IDLE;
  // Reset domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.6);

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
* FUNCTION    EEPROM_test
* DESCRIPTION Executes reads and writes to EEPROM to test the code
* PARAMETERS  none
* RETURNS     nothing
\*****************************************************************************/
void EEPROM_test(void)
{
  uint8 buffer[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  uint8 test[sizeof(buffer)];

  Analog_setDomain(MCU_DOMAIN,    FALSE);  // Does nothing
  Analog_setDomain(ANALOG_DOMAIN, TRUE);   // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);   // Enable I/O domain
  Analog_setDomain(COMMS_DOMAIN,  FALSE);  // Disable comms domain
  Analog_setDomain(SRAM_DOMAIN,   FALSE);  // Disable sram domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);   // Enable SPI domain
  Analog_setDomain(ENERGY_DOMAIN, FALSE);  // Disable energy domain
  Analog_setDomain(BUCK_DOMAIN7,  FALSE);  // Disable relay domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.65); // Set domain voltage to nominal (3.25V)
  Time_delay(1000); // Wait 1000ms for domains to settle

  // basic read test
  EEPROM_readEE((uint8*)0,test,sizeof(test));
  EEPROM_writeEE(buffer,(uint8*)0,sizeof(buffer));
  EEPROM_readEE((uint8*)0,test,sizeof(test));

  // boundary test
  EEPROM_writeEE(buffer,(uint8*)(8),sizeof(buffer));
  EEPROM_readEE((uint8*)(8),test,sizeof(test));

  while(1);
}
