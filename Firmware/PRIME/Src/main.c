#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

#include "stm32f429i_discovery_lcd.h"
#include "analog.h"
#include "eeprom.h"
#include "esp12.h"
#include "extusb.h"
#include "extmem.h"
#include "hih613x.h"
#include "i2c.h"
#include "m25px.h"
#include "plr5010d.h"
#include "powercon.h"
#include "sbt263.h"
#include "sdcard.h"
#include "si114x.h"
#include "spi.h"
#include "sst26.h"
#include "tests.h"
#include "time.h"

DAC_HandleTypeDef hdac;
DMA2D_HandleTypeDef hdma2d;
I2C_HandleTypeDef hi2c3;
LTDC_HandleTypeDef hltdc;
SPI_HandleTypeDef hspi5;
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;
SRAM_HandleTypeDef hsram1;
SDRAM_HandleTypeDef hsdram1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DAC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI5_Init(void);
static void MX_UART5_Init(void);
static void MX_USART1_UART_Init(void);

#include <stdlib.h>
#include <stdbool.h>

static struct
{
  bool vDomain0Test;
  bool vDomain1Test;
  bool vDomain2Test;
  bool ramTest;
  bool eepromTest;
  bool norFlashTest;
  bool nandFlashTest;
  bool sdCardTest;
  bool si114xTest;
  bool hih6130Test;
  bool usbTest;
  bool bluetoothTest;
  bool wifiTest;
  bool plr5010d0Test;
  bool plr5010d1Test;
  bool plr5010d2Test;
} sMain;

static uint8_t sdramTest[] = "SDRAM............";
static uint8_t vdom0Test[] = "Vdomain0.........";
static uint8_t vdom1Test[] = "Vdomain1.........";
static uint8_t vdom2Test[] = "Vdomain2.........";
static uint8_t eeTest[]    = "EEPROM...........";
static uint8_t norTest[]   = "NOR Flash........";
static uint8_t nandTest[]  = "NAND Flash.......";
static uint8_t sdTest[]    = "Micro-SD.........";
static uint8_t siTest[]    = "Si114x...........";
static uint8_t hihTest[]   = "HIH6130..........";
static uint8_t usbTest[]   = "USB Hi-Speed.....";
static uint8_t btTest[]    = "Bluetooth........";
static uint8_t wifiTest[]  = "ESP12-WiFi.......";
static uint8_t plr0Test[]  = "PLR5010D0........";
static uint8_t plr1Test[]  = "PLR5010D1........";
static uint8_t plr2Test[]  = "PLR5010D2........";

static void Display_DemoDescription(void)
{
  //uint8_t desc[50];
  
  /* Set LCD Foreground Layer  */
  BSP_LCD_SelectLayer(0);
  
  BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
  
  /* Clear the LCD */ 
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK); 
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  
  /* Set the LCD Text Color */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  
  /* Display LCD messages */
  BSP_LCD_DisplayStringAt(0, 10, (uint8_t*)"PRIME", CENTER_MODE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(0, 35, (uint8_t*)"Peripheral Tests", CENTER_MODE);
  
  /* Draw Bitmap */
//  BSP_LCD_DrawBitmap((BSP_LCD_GetXSize() - 80)/2, 65, (uint8_t *)stlogo);
//  BSP_LCD_SetFont(&Font8);
//  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()- 20, (uint8_t*)"Daniel Moore (drmoore@gmail.com)", CENTER_MODE);
//  
//  BSP_LCD_SetFont(&Font12);
//  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
//  BSP_LCD_FillRect(0, BSP_LCD_GetYSize()/2 + 15, BSP_LCD_GetXSize(), 60);
//  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
//  BSP_LCD_SetBackColor(LCD_COLOR_BLUE); 
//  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 + 30, (uint8_t*)"Press USER Button to start:", CENTER_MODE);
//  sprintf((char *)desc,"%s example", BSP_examples[DemoIndex].DemoName);
//  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2 + 45, (uint8_t *)desc, CENTER_MODE);   
}

void Main_printResult(uint32_t yPos, bool result)
{
  if (result)
  {
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)"[     ", RIGHT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)"OK  ", RIGHT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)" ]", RIGHT_MODE);
  }
  else
  {
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)"[     ", RIGHT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)"FAIL ", RIGHT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, yPos, (uint8_t *)"]", RIGHT_MODE);
  }
}

int main(void)
{
  bool runPOST = true;
  
  HAL_Init();  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  SystemClock_Config();  /* Configure the system clock */
  MX_GPIO_Init(); /* Initialize all configured peripherals */
  MX_DAC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_SPI5_Init();
  MX_UART5_Init();
  MX_USART1_UART_Init();
  BSP_LCD_Init();  // Initialize the LCD
  
  /* USER CODE BEGIN 2 */
  ExtMem_SDRAM_Initialization_sequence(REFRESH_COUNT, &hsdram1);
    
  Analog_init();             
  PowerCon_init(&hdac);      
  EEPROM_init();             
  ESP12_init(&huart1);     
  HIH613X_init();            
  M25PX_init();
  PLR5010D_init();  
  SBT263_init(&huart5);
  SST26_init();
  SDCard_init();
  SI114X_init();
    
  if (runPOST)
  {
    BSP_LED_On(LED3);
    BSP_LED_On(LED4);
  
    BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);  // Initialize the LCD Layers
    Display_DemoDescription();                      // Display test information
    
    BSP_LCD_DisplayStringAt(0, 60,  sdramTest, LEFT_MODE);
    sMain.ramTest       = ExtMem_testSDRAM();
    Main_printResult(60, sMain.ramTest);
    
    BSP_LCD_DisplayStringAt(0, 75,  vdom0Test, LEFT_MODE);
    sMain.vDomain0Test  = PowerCon_powerSupplyPOST(VOLTAGE_DOMAIN_0);
    Main_printResult(75, sMain.vDomain0Test);
    
    BSP_LCD_DisplayStringAt(0, 90,  vdom1Test, LEFT_MODE);
    sMain.vDomain1Test  = PowerCon_powerSupplyPOST(VOLTAGE_DOMAIN_1);
    Main_printResult(90, sMain.vDomain1Test);
    
    BSP_LCD_DisplayStringAt(0, 105,  vdom2Test, LEFT_MODE);
    sMain.vDomain2Test  = PowerCon_powerSupplyPOST(VOLTAGE_DOMAIN_2);
    Main_printResult(105, sMain.vDomain2Test);
    
    BSP_LCD_DisplayStringAt(0, 120,  eeTest, LEFT_MODE);
    sMain.eepromTest    = EEPROM_test();
    Main_printResult(120, sMain.eepromTest);
    
    BSP_LCD_DisplayStringAt(0, 135,  norTest, LEFT_MODE);
    sMain.norFlashTest  = M25PX_test();
    Main_printResult(135, sMain.norFlashTest);
    
    BSP_LCD_DisplayStringAt(0, 150,  nandTest, LEFT_MODE);
    sMain.nandFlashTest = SST26_test();
    Main_printResult(150, sMain.nandFlashTest);
    
    BSP_LCD_DisplayStringAt(0, 165,  sdTest, LEFT_MODE);
    sMain.sdCardTest = SDCard_test();
    Main_printResult(165, sMain.sdCardTest);
    
    BSP_LCD_DisplayStringAt(0, 180,  siTest, LEFT_MODE);
    sMain.si114xTest = SI114X_test();
    Main_printResult(180, sMain.si114xTest);
    
    BSP_LCD_DisplayStringAt(0, 195,  hihTest, LEFT_MODE);
    sMain.hih6130Test = HIH613X_test();
    Main_printResult(195, sMain.hih6130Test);
    
    BSP_LCD_DisplayStringAt(0, 210,  usbTest, LEFT_MODE);
    sMain.usbTest       = ExtUSB_testUSB();
    Main_printResult(210, sMain.usbTest);
    
    BSP_LCD_DisplayStringAt(0, 225,  btTest, LEFT_MODE);
    sMain.bluetoothTest = SBT263_test();
    Main_printResult(225, sMain.bluetoothTest);
    
    BSP_LCD_DisplayStringAt(0, 240,  wifiTest, LEFT_MODE);
    sMain.wifiTest = ESP12_test();
    Main_printResult(240, sMain.wifiTest);
      
    BSP_LCD_DisplayStringAt(0, 255,  plr0Test, LEFT_MODE);
    sMain.plr5010d0Test = PLR5010D_test(PLR5010D_DOMAIN0);
    Main_printResult(255, sMain.plr5010d0Test);
    
    BSP_LCD_DisplayStringAt(0, 270,  plr1Test, LEFT_MODE);
    sMain.plr5010d1Test = PLR5010D_test(PLR5010D_DOMAIN1);
    Main_printResult(270, sMain.plr5010d1Test);
    
    BSP_LCD_DisplayStringAt(0, 285,  plr2Test, LEFT_MODE);
    sMain.plr5010d2Test = PLR5010D_test(PLR5010D_DOMAIN2);
    Main_printResult(285, sMain.plr5010d2Test);
    
    if (sMain.bluetoothTest && sMain.eepromTest    && sMain.hih6130Test   && sMain.nandFlashTest &&
        sMain.norFlashTest  && sMain.plr5010d0Test && sMain.plr5010d1Test && sMain.plr5010d2Test &&
        sMain.ramTest       && sMain.sdCardTest    && sMain.sdCardTest    && sMain.si114xTest    && 
        sMain.usbTest       && sMain.vDomain0Test  && sMain.vDomain1Test  && sMain.vDomain2Test  &&
        sMain.wifiTest)
    {
      BSP_LCD_DisplayStringAt(0, 300,  "all systems go...", CENTER_MODE);
    }
    else
      BSP_LCD_DisplayStringAt(0, 300,  "an error occurred", CENTER_MODE);
    BSP_LED_Off(LED3);
    BSP_LED_Off(LED4);
  }
  BSP_LCD_DeInit();  // Turn off the LCD to reduce analog noise
  
  //__disable_irq();
  //HAL_SuspendTick();
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 3.3);
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 3.3);
  //uint16_t voltCode = 0;
  //while(1)
  //{
  //  PLR5010D_setVoltage(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, voltCode++);
  //}
  //double current;
  //while(1)
  //  for (current = 0; current < 125; current = current + .1)
  //    PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, current);
  //while (1)
  //{
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 12.5);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 25.0);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 50.0);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 100.0);
  //}
  //
  //while (1)
  //{
  //  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 1.8);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 50);
  //  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 2.3);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 50);
  //  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 2.8);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 50);
  //  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 3.3);
  //  PLR5010D_setCurrent(PLR5010D_DOMAIN2, PLR5010D_CHAN_BOTH, 50);
  //}

  while(1)
  {
    Tests_run();
  }
  
  while(1)
  {
    SI114X_ReadAndMeasure(NULL);
    Time_delay(100000);
  }
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* DAC init function */
void MX_DAC_Init(void)
{
  DAC_ChannelConfTypeDef sConfig;
  /** DAC Initialization **/
  hdac.Instance = DAC;
  HAL_DAC_Init(&hdac);
  /** DAC channel OUT2 config **/
  sConfig.DAC_Trigger = DAC_TRIGGER_SOFTWARE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2);
}

/* DMA2D init function */
void MX_DMA2D_Init(void)
{
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = CM_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  HAL_DMA2D_Init(&hdma2d);
  HAL_DMA2D_ConfigLayer(&hdma2d, 1);
}

/* I2C3 init function */
void MX_I2C3_Init(void)
{
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 400000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;
  HAL_I2C_Init(&hi2c3);
  I2C_init(&hi2c3);
}

/* SPI5 init function */
void MX_SPI5_Init(void)
{
  SPI_init(&hspi5);  // Reinitialize SPI after the LCD messes with it...
}

/* UART5 init function */
void MX_UART5_Init(void)
{
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart5);
}

/* USART1 init function */
void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
//  huart1.Init.BaudRate = 115200;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
}

/* FMC initialization function */
void MX_FMC_Init(void)
{
  /** Perform the initialization sequence for USB module on bank 2**/
  FMC_NORSRAM_TimingTypeDef Timing;
  hsram1.Instance = FMC_NORSRAM_DEVICE;
  hsram1.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
  hsram1.Init.NSBank = FMC_NORSRAM_BANK2;
  hsram1.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_8;
  hsram1.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FMC_EXTENDED_MODE_ENABLE;
  hsram1.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FMC_WRITE_BURST_DISABLE;
  hsram1.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ONLY;
  Timing.AddressSetupTime = 2;       // 2 Minimum empirically
  Timing.AddressHoldTime = 0;        // 0 Minimum empirically
  Timing.DataSetupTime = 7;          // 7 Minimum empirically
  Timing.BusTurnAroundDuration = 2;  // 2 Minimum empirically
  Timing.CLKDivision = 0;            // 0 Minimum empirically
  Timing.DataLatency = 0;            // 0 Minimum empirically
  Timing.AccessMode = FMC_ACCESS_MODE_A;
  HAL_SRAM_Init(&hsram1, &Timing, &Timing);

  /** Perform the SDRAM1 memory initialization sequence **/
  FMC_SDRAM_TimingTypeDef SdramTiming;
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 2;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;
  HAL_SDRAM_Init(&hsdram1, &SdramTiming);
}

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  __GPIOA_CLK_ENABLE();
  __GPIOB_CLK_ENABLE();
  __GPIOC_CLK_ENABLE();
  __GPIOD_CLK_ENABLE();
  __GPIOE_CLK_ENABLE();
  __GPIOF_CLK_ENABLE();
  __GPIOG_CLK_ENABLE();
  __GPIOH_CLK_ENABLE();

  /*Configure GPIO pins : PV_VSEL0_Pin PV_VSEL1_Pin _PV_CLR_Pin _PV_EN_Pin PV_VSEL2_Pin */
  GPIO_InitStruct.Pin = PV_VSEL0_Pin|PV_VSEL1_Pin|_PV_CLR_Pin|_PV_EN_Pin 
                          |PV_VSEL2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : _WF_RESET_Pin PV_D0_Pin PV_D1_Pin _BT_RESET_Pin */
  GPIO_InitStruct.Pin = _WF_RESET_Pin|PV_D0_Pin|PV_D1_Pin 
                          |_BT_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin TP_INT1_Pin */
  GPIO_InitStruct.Pin = B1_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : _PLR_CLR_Pin VADJ1_0_Pin VADJ1_1_Pin */
  GPIO_InitStruct.Pin = _PLR_CLR_Pin|VADJ1_0_Pin|VADJ1_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
	HAL_GPIO_WritePin(GPIOB, _USB_RST_Pin, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = _USB_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  // Should be OD output
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : TE_Pin */
  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RDX_Pin WRX_DCX_Pin */
  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  
  /*Configure GPIO pins : LD3_Pin LD4_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}
