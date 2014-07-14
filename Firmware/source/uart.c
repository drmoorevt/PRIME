#include "stm32f2xx.h" // Used for manipulating base UART registers
#include "dma.h"
#include "gpio.h"      // Used for setting up the pins as UART
#include "time.h"      // Used for timeout functionality
#include "util.h"
#include "uart.h"

#define FILE_ID UART_C

// PortInfo contains all of the information needed to configure the clocks and pins for
// initializing a generic UART port. It is populated via lookup in UART_getPortInfo()
typedef struct
{
  struct
  {
    USART_TypeDef   *pUART;     // The base register for hardware UART interaction
    volatile uint32 *pClockReg; // The clock register for enabling/disabling clocks to the UART
    volatile uint32 *pResetReg; // The reset register for resetting the UART
    uint32    clockEnableBit;   // The bit mask used for enabling the clock to the UART
    uint32    resetBit;         // The bit mask used for resetting the UART
    IRQn_Type irqNumber;        // The irqNumber associated with this UART
    DMA_TypeDef *dmaController; // The dmaController associated with this UART
  } periph;

  GPIO_TypeDef       *rxPinPort;
  GPIO_InitTypeDef    rxPinConfig;
  DMA_Stream_TypeDef *rxStreamNumDMA;
  uint32              rxChanNumDMA;
  IRQn_Type           rxCompleteIRQ;

  GPIO_TypeDef       *txPinPort;
  GPIO_InitTypeDef    txPinConfig;
  DMA_Stream_TypeDef *txStreamNumDMA;
  uint32              txChanNumDMA;
  IRQn_Type           txCompleteIRQ;
} PortInfo;

// CommStatus is maintained internally but toSend and toReceive are populated via appLayer requests
typedef struct
{
  uint16 bytesSent;
  uint16 bytesToSend;
  uint16 bytesToReceive;
  uint16 bytesReceived;
} CommStatus;

// CommPort contains the information required for an appLayer agent to send and receive data
typedef struct
{
  boolean         isConfigured; // Indicates if this port is registered to an appLayer
  USART_TypeDef   *pUART;       // The base register for hardware UART interaction
  AppCommConfig   appConfig;    // Buffers and callbacks set by the appLayer
  CommStatus      commStatus;   // State information for sending and receiving
  SoftTimer       timer;        // The soft timer to be used with this port
  SoftTimerConfig icConfig;     // Configuration used for inter char timeouts
} CommPort;

static struct
{
  CommPort port[UART_PORT_MAX];
} sUART;

static UARTPort UART_getPortHandle(SoftTimer timer);
static SoftTimer UART_getTimerHandle(UARTPort port);
static void UART_notifyTimeout(SoftTimer timer);
static uint16 Uart_calcBaudRateRegister(BaudRate baud);

/*************************************************************************************************\
* FUNCTION    UART_init
* DESCRIPTION Initializes the selected port to its default configuration
* PARAMETERS  port: The UART port to initialize and configure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void UART_init(void)
{
  Util_fillMemory(&sUART, sizeof(sUART), 0x00);
}

/*************************************************************************************************\
* FUNCTION    UART_getPortInfo
* DESCRIPTION Retrieves pointers and bits from stm32f2xx.h to be used in a generic format
* PARAMETERS  UARTPort: The UART port to find information for
* RETURNS     PortInfo: The consolidated peripheral information
* NOTES       None
\*************************************************************************************************/
static boolean UART_getPortInfo(UARTPort port, PortInfo *portInfo)
{
  switch (port)
  {
    case USART_PORT1:
      portInfo->periph.pUART              = USART1;
      portInfo->periph.pClockReg          = &RCC->APB2ENR;
      portInfo->periph.clockEnableBit     = RCC_APB2ENR_USART1EN;
      portInfo->periph.pResetReg          = &RCC->APB2RSTR;
      portInfo->periph.resetBit           = RCC_APB2RSTR_USART1RST;
      portInfo->periph.irqNumber          = USART1_IRQn;
      portInfo->periph.dmaController      = DMA2;
    
      portInfo->rxPinPort = GPIOB;
      GPIO_structInitUART(&portInfo->rxPinConfig, GPIO_Pin_7, port);
      portInfo->rxStreamNumDMA = DMA2_Stream5;
      portInfo->rxChanNumDMA   = DMA_Channel_4;
      portInfo->rxCompleteIRQ  = DMA2_Stream5_IRQn;
    
      portInfo->txPinPort = GPIOB;
      GPIO_structInitUART(&portInfo->txPinConfig, GPIO_Pin_6, port);
      portInfo->txStreamNumDMA = DMA2_Stream7;
      portInfo->txChanNumDMA   = DMA_Channel_4;
      portInfo->txCompleteIRQ  = DMA2_Stream7_IRQn;
      break;
    case USART_PORT3:
      portInfo->periph.pUART              = USART3;
      portInfo->periph.pClockReg          = &RCC->APB1ENR;
      portInfo->periph.clockEnableBit     = RCC_APB1ENR_USART3EN;
      portInfo->periph.pResetReg          = &RCC->APB1RSTR;
      portInfo->periph.resetBit           = RCC_APB1RSTR_USART3RST;
      portInfo->periph.irqNumber          = USART3_IRQn;
      portInfo->periph.dmaController      = DMA1;
    
      portInfo->rxPinPort = GPIOB;
      GPIO_structInitUART(&portInfo->rxPinConfig, GPIO_Pin_11, port);
      portInfo->rxStreamNumDMA = DMA1_Stream1;
      portInfo->rxChanNumDMA   = DMA_Channel_4;
      portInfo->rxCompleteIRQ  = DMA1_Stream1_IRQn;
    
      portInfo->txPinPort = GPIOB;
      GPIO_structInitUART(&portInfo->txPinConfig, GPIO_Pin_10, port);
      portInfo->txStreamNumDMA = DMA1_Stream3;
      portInfo->txChanNumDMA   = DMA_Channel_4;
      portInfo->txCompleteIRQ  = DMA1_Stream3_IRQn;
      break;
    case UART_PORT4:
      portInfo->periph.pUART              = UART4;
      portInfo->periph.pClockReg          = &RCC->APB1ENR;
      portInfo->periph.clockEnableBit     = RCC_APB1ENR_UART4EN;
      portInfo->periph.pResetReg          = &RCC->APB1RSTR;
      portInfo->periph.resetBit           = RCC_APB1RSTR_UART4RST;
      portInfo->periph.irqNumber          = UART4_IRQn;
      portInfo->periph.dmaController      = DMA1;
    
      portInfo->rxPinPort = GPIOC;
      GPIO_structInitUART(&portInfo->rxPinConfig, GPIO_Pin_11, port);
      portInfo->rxStreamNumDMA = DMA1_Stream2;
      portInfo->rxChanNumDMA   = DMA_Channel_4;
      portInfo->rxCompleteIRQ  = DMA1_Stream2_IRQn;
    
      portInfo->txPinPort = GPIOC;
      GPIO_structInitUART(&portInfo->txPinConfig, GPIO_Pin_10, port);
      portInfo->txStreamNumDMA = DMA1_Stream4;
      portInfo->txChanNumDMA   = DMA_Channel_4;
      portInfo->txCompleteIRQ  = DMA1_Stream4_IRQn;
      break;
    case UART_PORT5:
      portInfo->periph.pUART              = UART5;
      portInfo->periph.pClockReg          = &RCC->APB1ENR;
      portInfo->periph.clockEnableBit     = RCC_APB1ENR_UART5EN;
      portInfo->periph.pResetReg          = &RCC->APB1RSTR;
      portInfo->periph.resetBit           = RCC_APB1RSTR_UART5RST;
      portInfo->periph.irqNumber          = UART5_IRQn;
      portInfo->periph.dmaController      = DMA1;
    
      portInfo->rxPinPort = GPIOD;
      GPIO_structInitUART(&portInfo->rxPinConfig, GPIO_Pin_2, port);
      portInfo->rxStreamNumDMA = DMA1_Stream0;
      portInfo->rxChanNumDMA   = DMA_Channel_4;
      portInfo->rxCompleteIRQ  = DMA1_Stream0_IRQn;
    
      portInfo->txPinPort = GPIOC;
      GPIO_structInitUART(&portInfo->txPinConfig, GPIO_Pin_12, port);
      portInfo->txStreamNumDMA = DMA1_Stream7;
      portInfo->txChanNumDMA   = DMA_Channel_4;
      portInfo->txCompleteIRQ  = DMA1_Stream7_IRQn;
      break;
    case USART_PORT6:
      portInfo->periph.pUART              = USART6;
      portInfo->periph.pClockReg          = &RCC->APB2ENR;
      portInfo->periph.clockEnableBit     = RCC_APB2ENR_USART6EN;
      portInfo->periph.pResetReg          = &RCC->APB2RSTR;
      portInfo->periph.resetBit           = RCC_APB2RSTR_USART6RST;
      portInfo->periph.irqNumber          = USART6_IRQn;
      portInfo->periph.dmaController      = DMA2;
    
      portInfo->rxPinPort = GPIOC;
      GPIO_structInitUART(&portInfo->rxPinConfig, GPIO_Pin_7, port);
      portInfo->rxStreamNumDMA = DMA2_Stream1;
      portInfo->rxChanNumDMA   = DMA_Channel_5;
      portInfo->rxCompleteIRQ  = DMA2_Stream1_IRQn;
    
      portInfo->txPinPort = GPIOC;
      GPIO_structInitUART(&portInfo->txPinConfig, GPIO_Pin_6, port);
      portInfo->txStreamNumDMA = DMA2_Stream6;
      portInfo->txChanNumDMA   = DMA_Channel_5;
      portInfo->txCompleteIRQ  = DMA2_Stream6_IRQn;
      break;
    default:
      return ERROR;
  }
  return SUCCESS;
}

/*************************************************************************************************\
* FUNCTION    UART_openPort
* DESCRIPTION Retrieves pointers and bits from stm32f2xx.h to be used in a generic format
* PARAMETERS  UARTPort: The UART port to find information for
* RETURNS     PortInfo: The consolidated peripheral information
* NOTES       None
\*************************************************************************************************/
boolean UART_openPort(UARTPort port, AppCommConfig config)
{
  PortInfo pi;
  UARTPort portToOpen = port;

  if (ERROR == UART_getPortInfo(port, &pi))
    return ERROR; // Port does not exist

  if (sUART.port[portToOpen].isConfigured)
    return ERROR; // Port is already open, return error code

  /***** Configure the peripheral *****/
  *pi.periph.pClockReg |= pi.periph.clockEnableBit; // Enable port clocks
  *pi.periph.pResetReg |= pi.periph.resetBit;       // Reset the peripheral
  *pi.periph.pResetReg &= ~pi.periph.resetBit;      // Then release it from reset
   pi.periph.pUART->CR1 |= USART_CR1_RXNEIE;        // Enable the RX not empty int
   pi.periph.pUART->CR1 &= ~USART_CR1_TXEIE;        // Disable the tx data reg empty int
   pi.periph.pUART->CR1 &= ~USART_CR1_TCIE;         // Disable the tx complete int
   pi.periph.pUART->CR2 &= 0xFFFF0000;              // Unnecessary, but for consistency
   pi.periph.pUART->CR3 &= 0xFFFF0000;              // No flow control
   pi.periph.pUART->CR3 |= USART_CR3_DMAT;          // Enable DMA transmissions
   pi.periph.pUART->CR3 |= USART_CR3_DMAR;          // Enable DMA receptions
   pi.periph.pUART->BRR  = Uart_calcBaudRateRegister(config.UARTConfig.baud);
   pi.periph.pUART->CR1 |= USART_CR1_UE;            // Enable UART

  /***** Configure the RX GPIO Pin *****/
  GPIO_setPortClock(pi.rxPinPort, TRUE); // Enable pin clocks
  GPIO_configurePins(pi.rxPinPort, &pi.rxPinConfig);

  /***** Configure the TX GPIO Pin *****/
  GPIO_setPortClock(pi.txPinPort, TRUE); // Enable pin clocks
  GPIO_configurePins(pi.txPinPort, &pi.txPinConfig);

  /***** Set the App layer buffers, callbacks, timer and configured *****/
  Util_copyMemory((uint8*)&config, (uint8*)&sUART.port[portToOpen].appConfig, sizeof(config));
  sUART.port[portToOpen].timer = UART_getTimerHandle(portToOpen);
  sUART.port[portToOpen].pUART = pi.periph.pUART;
  sUART.port[portToOpen].isConfigured = TRUE;

  return SUCCESS;
}

/*****************************************************************************\
* FUNCTION    UART_getTimerHandle
* DESCRIPTION Grabs the timer associated with the supplied UART
* PARAMETERS  port: The port for which to grab to timer handle
* RETURNS     The timeout timer associated with the port
* NOTES       None
\*****************************************************************************/
static SoftTimer UART_getTimerHandle(UARTPort port)
{
  switch (port)
  {
    case USART_PORT1: return TIME_SOFT_TIMER_USART1;
    case USART_PORT2: return TIME_SOFT_TIMER_USART2;
    case USART_PORT3: return TIME_SOFT_TIMER_USART3;
    case UART_PORT4:  return TIME_SOFT_TIMER_UART4;
    case UART_PORT5:  return TIME_SOFT_TIMER_UART5;
    case USART_PORT6: return TIME_SOFT_TIMER_USART6;
    default:          return TIME_SOFT_TIMER_MAX; // Error condition
  }
}

/*****************************************************************************\
* FUNCTION    UART_getPortHandle
* DESCRIPTION Grabs the UART associated with a provided timer
* PARAMETERS  timer: the timer for which to grab the port
* RETURNS     The port on which the timer is associated
* NOTES       None
\*****************************************************************************/
static UARTPort UART_getPortHandle(SoftTimer timer)
{
  switch (timer)
  {
    case TIME_SOFT_TIMER_USART1: return USART_PORT1;
    case TIME_SOFT_TIMER_USART2: return USART_PORT2;
    case TIME_SOFT_TIMER_USART3: return USART_PORT3;
    case TIME_SOFT_TIMER_UART4:  return UART_PORT4;
    case TIME_SOFT_TIMER_UART5:  return UART_PORT5;
    case TIME_SOFT_TIMER_USART6: return USART_PORT6;
    default:                     return UART_PORT_MAX; // Error condition
  }
}

/**************************************************************************************************\
* FUNCTION    UART_sendData
* DESCRIPTION Writes character to the indicated serial port
* PARAMETERS  port: The port over which to transmit
*             pSrc: Source of the data to send
*             numBytes: The number of bytes at which to trigger the transmit complete function
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean UART_sendData(UARTPort port, uint8 *pSrc, uint16 numBytes)
{
  PortInfo portInfo;
  DMA_InitTypeDef dmaInit;
  
  sUART.port[port].appConfig.appTransmitBuffer = pSrc; // Store the source just in case

//  if (sUART.port[port].commStatus.bytesToSend != 0)
//    return FALSE;  // UART is currently transmitting
  
  UART_getPortInfo(port, &portInfo);
//  DMA_DeInit(portInfo.txStreamNumDMA);
  
  /***** Configure DMA TX transfers *****/
  DMA_StructInit(&dmaInit);
  dmaInit.DMA_Channel = portInfo.txChanNumDMA;
  dmaInit.DMA_PeripheralBaseAddr = (uint32)&sUART.port[port].pUART->DR;
  dmaInit.DMA_Memory0BaseAddr = (uint32)pSrc;
  dmaInit.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  dmaInit.DMA_BufferSize = numBytes;
  dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dmaInit.DMA_Priority = DMA_Priority_Medium;
  dmaInit.DMA_FIFOMode = DMA_FIFOMode_Enable;

  /***** Enable DMA TX transfers *****/
  DMA_Init(portInfo.txStreamNumDMA, &dmaInit);
  DMA_ITConfig(portInfo.txStreamNumDMA, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE, ENABLE);
  NVIC_EnableIRQ(portInfo.txCompleteIRQ);
  DMA_Cmd(portInfo.txStreamNumDMA, ENABLE, sUART.port[port].appConfig.appNotifyCommsEvent);
     
  sUART.port[port].pUART->CR1 |= USART_CR1_TE;            // Enable TX
//  sUART.port[port].commStatus.bytesToSend  = numBytes;
//  sUART.port[port].commStatus.bytesSent = 0;
  
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    UART_receiveData
* DESCRIPTION Reads characters from the indicated serial port
* PARAMETERS  port: The port over which to receive
              numBytes: The number of bytes at which to trigger the receive complete function
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
boolean UART_receiveData(UARTPort port, uint32 numBytes, uint32 timeout, boolean interChar)
{
  PortInfo portInfo;
  SoftTimerConfig timer;
  DMA_InitTypeDef dmaInit;
  
  UART_getPortInfo(port, &portInfo);
  
  /***** Configure the timeout (or inter-char timeout) timer for this port *****/
  timer.appNotifyTimerExpired = &UART_notifyTimeout;  
  timer.timer  = sUART.port[port].timer;
  timer.value  = timeout;
  timer.reload = 0;
  sUART.port[port].icConfig = timer;
  
  /***** Configure DMA RX transfers *****/
  DMA_StructInit(&dmaInit);
  dmaInit.DMA_Channel = portInfo.rxChanNumDMA;
  dmaInit.DMA_PeripheralBaseAddr = (uint32)&sUART.port[port].pUART->DR;
  dmaInit.DMA_Memory0BaseAddr = (uint32)sUART.port[port].appConfig.appReceiveBuffer;
  dmaInit.DMA_BufferSize = numBytes;
  dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dmaInit.DMA_Priority = DMA_Priority_Medium;
  dmaInit.DMA_FIFOMode = DMA_FIFOMode_Enable;

  /***** Enable DMA RX transfers *****/
  DMA_Init(portInfo.rxStreamNumDMA, &dmaInit);
  DMA_ITConfig(portInfo.rxStreamNumDMA, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE, ENABLE);
  DMA_ClearITPendingBit(portInfo.rxStreamNumDMA, DMA_IT_TCIF0 | DMA_IT_TEIF0 | DMA_IT_FEIF0);
  NVIC_EnableIRQ(portInfo.rxCompleteIRQ);
  DMA_Cmd(portInfo.rxStreamNumDMA, ENABLE, sUART.port[port].appConfig.appNotifyCommsEvent);
  
  sUART.port[port].commStatus.bytesToReceive  = numBytes;
  sUART.port[port].commStatus.bytesReceived   = 0;
  
  sUART.port[port].pUART->CR1 |= USART_CR1_RXNEIE;  // Turn on the rx interrupt
  sUART.port[port].pUART->CR1 |= USART_CR1_RE;      // Enable RX

  if (TRUE == interChar)
  {
    timer.value = 0; // stop the timer
    Time_startTimer(timer);
    NVIC_EnableIRQ(portInfo.periph.irqNumber); // Wait for the first byte to start ICtimer
  }
  else
  {
    NVIC_DisableIRQ(portInfo.periph.irqNumber); // Ensure the IC interrupt is turned off
    Time_startTimer(timer);
  }

  
  return SUCCESS;
}

/**************************************************************************************************\
* FUNCTION    UART_notifyTimeout
* DESCRIPTION The timer module will inform us in interrupt context of timeouts
*             Will turn off reception on the appropriate port
* PARAMETERS  port: The port which has timed out
* RETURNS     Nothing
* NOTES       None
\**************************************************************************************************/
void UART_notifyTimeout(SoftTimer timer)
{
  uint32 bytesReceived;
  UARTPort port = UART_getPortHandle(timer);
  bytesReceived = UART_stopReceive(port);
  sUART.port[port].appConfig.appNotifyCommsEvent(COMMS_EVENT_RX_TIMEOUT, bytesReceived);
}

/**************************************************************************************************\
* FUNCTION    UART_stopReceive
* DESCRIPTION Allows the applayer to abort a reception operation
* PARAMETERS  port: The port to stop receiving on
* RETURNS     The number of bytes received before stopping
* NOTES       None
\**************************************************************************************************/
uint32 UART_stopReceive(UARTPort port)
{
  PortInfo portInfo;
  uint32 bytesReceived;
  
  SoftTimerConfig nullTimer = {TIME_SOFT_TIMER_USART1, 0, 0, 0};
  nullTimer.timer = sUART.port[port].timer;
  Time_startTimer(nullTimer); // Clear the UART timeout timer by 'starting' it zeroed out

  UART_getPortInfo(port, &portInfo);
  // Bytes received is the number we intended on receiving minus the number remaining
  bytesReceived = sUART.port[port].commStatus.bytesToReceive -
                  portInfo.rxStreamNumDMA->NDTR;
  
  NVIC_DisableIRQ(portInfo.rxCompleteIRQ);
  DMA_ITConfig(portInfo.rxStreamNumDMA, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE, DISABLE);
  DMA_ClearITPendingBit(portInfo.rxStreamNumDMA, DMA_IT_TCIF0 | DMA_IT_TEIF0 | DMA_IT_FEIF0);
  DMA_Cmd(portInfo.rxStreamNumDMA, DISABLE, sUART.port[port].appConfig.appNotifyCommsEvent);
  DMA_DeInit(portInfo.rxStreamNumDMA);
  
  NVIC_DisableIRQ(portInfo.periph.irqNumber);
  sUART.port[port].pUART->CR1 &= (~USART_CR1_RXNEIE);  // Turn off the rx interrupt
  sUART.port[port].pUART->CR1 &= (~USART_CR1_RE);      // Disable RX
  
  sUART.port[port].commStatus.bytesToReceive = 0;
  sUART.port[port].commStatus.bytesReceived  = 0;
  
  return bytesReceived;
}

/*************************************************************************************************\
* FUNCTION    Uart_calculateBaudRateRegister
* DESCRIPTION Calculates the value for the baud rate register
* PARAMETERS  baud: The baud rate for which to calculate the BRR
* RETURNS     The appropriate value for the BRR
* NOTES       None
\*************************************************************************************************/
static uint16 Uart_calcBaudRateRegister(BaudRate baud)
{
  switch (baud)
  {
    case UART_BAUDRATE_2400:
      return 0x30C0;  // 780
    case UART_BAUDRATE_4800:
      return 0x1860;  // 390
    case UART_BAUDRATE_9600:
      return 0x0C30;  // 195
    case UART_BAUDRATE_19200:
      return 0x0618;  // 97.5
    case UART_BAUDRATE_38400:
      return 0x030C;  // 48.75
    case UART_BAUDRATE_57600:
      return 0x0208;  // 32.5
    case UART_BAUDRATE_115200:
      return 0x0104;  // 16.25
    case UART_BAUDRATE_230400:
      return 0x0082;  // 8.125
    case UART_BAUDRATE_460800:
      return 0x0041;  // 4.0625
    case UART_BAUDRATE_921600:
      return 0x0021;  // 2.0625
    default:
      return 0;
  }
}

/*****************************************************************************\
* FUNCTION    UART_handleInterrupt
* DESCRIPTION Generic handler for UART interrupts
* PARAMETERS  port: The port on which the interrupt occurred
* RETURNS     Nothing
* NOTES       This should be broken up into multiple functions
\*****************************************************************************/
void UART_handleInterrupt(UARTPort port)
{
  Time_startTimer(sUART.port[port].icConfig);
}

/*****************************************************************************\
* FUNCTION    UARTx_IRQHandler
* DESCRIPTION Writes character to the indicated serial port
* PARAMETERS  None
* RETURNS     Nothing
\*****************************************************************************/
void USART1_IRQHandler(void)
{
  UART_handleInterrupt(USART_PORT1);
}

void USART2_IRQHandler(void)
{
  UART_handleInterrupt(USART_PORT2);
}

void USART3_IRQHandler(void)
{
  UART_handleInterrupt(USART_PORT3);
}

void UART4_IRQHandler(void)
{
  UART_handleInterrupt(UART_PORT4);
}

void UART5_IRQHandler(void)
{
  UART_handleInterrupt(UART_PORT5);
}

void USART6_IRQHandler(void)
{
  UART_handleInterrupt(USART_PORT6);
}
