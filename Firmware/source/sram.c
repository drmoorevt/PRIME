#include "stm32f2xx.h"
#include "fsmc.h"
#include "gpio.h"
#include "rcc.h"
#include "sram.h"

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
  p.FSMC_AddressSetupTime = 1; // 5
  p.FSMC_AddressHoldTime = 0;
  p.FSMC_DataSetupTime = 1; // 5
  p.FSMC_BusTurnAroundDuration = 0;
  p.FSMC_CLKDivision = 0;
  p.FSMC_DataLatency = 0;
  p.FSMC_AccessMode = FSMC_AccessMode_A;

  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1; // changed
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Enable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

  /*!< Enable FSMC Bank1_SRAM2 Bank */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE); // changed
  
  return TRUE;
}

boolean SRAM_test(void)
{
  uint32 ramAddr;
  for (ramAddr = 0; ramAddr < SRAM_ADDR; ramAddr++)
  {
    GPSRAM->extmem[ramAddr] = (uint16)ramAddr;
    if (GPSRAM->extmem[ramAddr] != (uint16)ramAddr)
      return FALSE;
  }
  return TRUE;
}
