#include "stm32f2xx.h"
#include "fsmc.h"
#include "gpio.h"
#include "rcc.h"
#include "sram.h"
#include "stdio.h"
#include "time.h"
#include "util.h"

#define FILE_ID SRAM_C

/**************************************************************************************************\
* FUNCTION     SRAM_init
* DESCRIPTION  Initializes SRAM via FSMC support
* PARAMETERS   none
* RETURNS      nothing
\**************************************************************************************************/
void SRAM_init(void)
{
  SRAM_setup(SRAM_STATE_ENABLED);
}

// These macros will only apply to the SPI version of the chip
#define SELECT_WRITE()   do { GPIOD->BSRRH |= 0x00000020; Time_delay(1); } while (0)
#define DESELECT_WRITE() do { GPIOD->BSRRL |= 0x00000020; Time_delay(1); } while (0)

#define SELECT_READ()   do { GPIOD->BSRRH |= 0x00000010; Time_delay(0); } while (0)
#define DESELECT_READ() do { GPIOD->BSRRL |= 0x00000010; Time_delay(0); } while (0)

#define SELECT_CHIP()   do { GPIOD->BSRRH |= 0x00000080; Time_delay(0); } while (0)
#define DESELECT_CHIP() do { GPIOD->BSRRL |= 0x00000080; Time_delay(0); } while (0)

#define SELECT_LOWNIB()   do { GPIOD->BSRRL |= 0x00004000; } while (0)
#define SELECT_HINIB()    do { GPIOE->BSRRL |= 0x00000200; } while (0)

void SRAM_sendByte(uint8 byte)
{
    GPIO_InitTypeDef GPIO_InitStructure;
  /* General pin configuration */
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_AltFunc = GPIO_AF_SYSTEM;
  
  /* GPIOD configuration */
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4  | GPIO_Pin_5  | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_14  | 
                                 GPIO_Pin_15;
  GPIO_configurePins(GPIOD, &GPIO_InitStructure);  
  
  /* GPIOE configuration */
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_7  | GPIO_Pin_8  | GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_configurePins(GPIOE, &GPIO_InitStructure);
  
  DESELECT_READ();
  
  SELECT_CHIP();
  SELECT_LOWNIB();
  SELECT_HINIB();
  SELECT_WRITE();
  DESELECT_WRITE();
  DESELECT_CHIP();
}

/**************************************************************************************************\
* FUNCTION     SRAM_setup
* DESCRIPTION
* PARAMETERS   none
* RETURNS      TRUE
\**************************************************************************************************/
boolean SRAM_setup(SRAMState state)
{
  
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  p;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOs clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF |
                         RCC_AHB1Periph_GPIOG, ENABLE);

  /* Enable FSMC clock */
  RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);

  /* General pin configuration */
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_AltFunc = GPIO_AF_FSMC;
  
  /* GPIOD configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_4  |
                                GPIO_Pin_5  | GPIO_Pin_7  | GPIO_Pin_8  | 
                                GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 | 
                                GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_configurePins(GPIOD, &GPIO_InitStructure);

  /* GPIOE configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7  | 
                                GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 | 
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | 
                                GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_configurePins(GPIOE, &GPIO_InitStructure);

  /* GPIOF configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  |
                                GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  |
                                GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 |
                                GPIO_Pin_15;
  GPIO_configurePins(GPIOF, &GPIO_InitStructure);

  /* GPIOG configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  |
                                GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  |
                                GPIO_Pin_9;
  GPIO_configurePins(GPIOG, &GPIO_InitStructure);

  /*-- FSMC Configuration ------------------------------------------------------*/
  p.FSMC_AddressSetupTime = 0x1; // 5
  p.FSMC_AddressHoldTime = 0x4;
  p.FSMC_DataSetupTime = 0x1; // 5
  p.FSMC_BusTurnAroundDuration = 0x2;
  p.FSMC_CLKDivision = 0x0;
  p.FSMC_DataLatency = 0x2;
  p.FSMC_AccessMode = FSMC_AccessMode_A;

  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1; // changed
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;


  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

  /*!< Enable FSMC Bank1_SRAM2 Bank */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE); // changed
  
    /* General pin configuration */
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_AltFunc = GPIO_AF_SYSTEM;
  
  /* SIWU# */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_configurePins(GPIOD, &GPIO_InitStructure);
  
  return TRUE;
}

#define FT232H_DATA_AVAILABLE  (0x01)
#define FT232H_SPACE_AVAILABLE (~0x02)
#define FT232H_SUSPEND         (0x04)
#define FT232H_CONFIGURED      (0x08)
#define FT232H_BUFFER_SIZE     (1024)
uint8 volatile * const pDataPipe = (uint8 *)0x60000000;
uint8 volatile * const pStatPipe = (uint8 *)0x60010000;

#define SELECT_SIWU()   do { GPIOD->BSRRH |= 0x00000040; } while (0)
#define DESELECT_SIWU() do { GPIOD->BSRRL |= 0x00000040; } while (0)

boolean SRAM_test(void)
{
  uint32 i;
  uint32 size;
  uint32 counter = 0;
  volatile uint32 j;
  volatile uint8 status;
  volatile uint8 inChar;
  char helloString[FT232H_BUFFER_SIZE];
  
  DISABLE_SYSTICK_INTERRUPT();
  
  /*
  while(1)
  {
    DESELECT_SIWU();
    for (i = 0; i < 1024; i++)
      *(pDataPipe) = 'A';
    SELECT_SIWU();
    for (j=0; j < 2; j++);
  }
  */
  
  while(1)
  {
    if ((*pStatPipe & 0x0F) & FT232H_SPACE_AVAILABLE)
    {
      DESELECT_SIWU();
      while ((*pStatPipe & 0x0F) & FT232H_SPACE_AVAILABLE)
        *(pDataPipe) = 'A';
      SELECT_SIWU();
    }
  }
  
  i = 0;
  while (i < (FT232H_BUFFER_SIZE - 16))
    i += sprintf(&helloString[i], "%d \r\n", counter++);
  
  while (1)
  {
    for (i = 0; i < FT232H_BUFFER_SIZE; i++)
    {
      while ((*pStatPipe & 0x0F) & FT232H_SPACE_AVAILABLE)
      {
        DESELECT_SIWU();
        //*(pDataPipe) = helloString[i];
        *pDataPipe = 'A';
      }
      SELECT_SIWU();
      //*(pStatPipe) = 'A';
    }
  }
  
  while (1)
  {
    status = (*pStatPipe & 0x0F);
    if (status & FT232H_CONFIGURED)
    {
      if (status & FT232H_DATA_AVAILABLE)
      {
        while (status & FT232H_DATA_AVAILABLE)
        {
          inChar = *pDataPipe;
          Util_fillMemory(helloString, sizeof(helloString), 0x00);
          size = sprintf(helloString, "inChar = %c\r\n", inChar);
          for (i = 0; i < size; i++)
            *(pDataPipe) = helloString[i];
          status = (*pStatPipe & 0x0F);
        }
        for (j = 0; j < 10000000; j++);
      }
      else if (status & FT232H_SPACE_AVAILABLE)
      {
        size = sprintf(helloString, "Status = %X, Counter = %d \r\n", status, counter++);
        for (i = 0; i < size; i++)
          *(pDataPipe) = helloString[i];
      }
    }
  }
//  for (ramAddr = 0; ramAddr < SRAM_ADDR; ramAddr++)
//  {
//    *(uint16 *)(0x60000000 + ramAddr) = 0xDEAD;
//    GPSRAM->extmem[ramAddr] = 0xDEAD;
//    GPSRAM->extmem[ramAddr] = (uint16)ramAddr * (ramAddr - 1);
//    if (GPSRAM->extmem[ramAddr] != (uint16)ramAddr * (ramAddr - 1))
//      return FALSE;
//  }
  return TRUE;
}
