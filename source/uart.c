#include "stm32f2xx.h"
#include "util.h"
#include "uart.h"

#define FILE_ID UART_C

typedef struct ComDevice
{
  const uint32 devAddr;
  IOCompletionRoutine ioCompletionRoutine;
  uint8* pReadBuffer;
  uint16 readLen;
  const uint8* pWriteBuffer;
  uint16 writeLen;
  void* pUserData;
  IOState state;
  IOResult lastResult;
  uint8 dmaRxChannel;
  uint8 dmaTxChannel;
} CommDevice;

struct
{
  FullDuplexPort U1;
  FullDuplexPort U2;
  FullDuplexPort U3;
  FullDuplexPort U4;
  FullDuplexPort U5;
  FullDuplexPort U6;
} sUART;

/*************************************************************************************************\
* FUNCTION    UART_init
* DESCRIPTION Initializes the selected port to its default configuration
* PARAMETERS  port: The UART port to initialize and configure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
uint16 Uart_calculateBaudRateRegister(BaudRate baud)
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
    default:
      return 0;
  }
}

/*************************************************************************************************\
* FUNCTION    UART_init
* DESCRIPTION Initializes the selected port to its default configuration
* PARAMETERS  port: The UART port to initialize and configure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
boolean UART_init(SerialPort port, BaudRate baud, boolean enableInterrupt)
{
  USART_TypeDef  *pUARTConfig;
  FullDuplexPort *pUARTPort;
  IRQn_Type       irqNumber;

  switch (port)
  {
    case SERIAL_PORT1:
      pUARTConfig = UART1;
      pUARTPort   = &sUART.U1;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART1EN);   // Enable UART1 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART1RST); // Reset UART1
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART1RST); // Release UART1 from reset
      irqNumber = UART1_IRQn;
      break;
    case SERIAL_PORT2:
      pUARTConfig = UART2;
      pUARTPort   = &sUART.U2;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART2EN);   // Enable UART2 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART2RST); // Reset UART2
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART2RST); // Release UART2 from reset
      irqNumber = UART2_IRQn;
      break;
    case SERIAL_PORT3:
      pUARTConfig = UART3;
      pUARTPort   = &sUART.U3;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART3EN);   // Enable UART3 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART3RST); // Reset UART3
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART3RST); // Release UART3 from reset
      irqNumber = UART3_IRQn;
      break;
    case SERIAL_PORT4:
      pUARTConfig = UART4;
      pUARTPort   = &sUART.U4;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART4EN);   // Enable UART4 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART4RST); // Reset UART4
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART4RST); // Release UART4 from reset
      irqNumber = UART4_IRQn;
      break;
    case SERIAL_PORT5:
      pUARTConfig = UART5;
      pUARTPort   = &sUART.U5;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART5EN);   // Enable UART5 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART5RST); // Reset UART5
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART5RST); // Release UART5 from reset
      irqNumber = UART5_IRQn;
      break;
    case SERIAL_PORT6:
      pUARTConfig = UART6;
      pUARTPort   = &sUART.U6;
      // TODO: Set port pins to alternate function here
      SET_BIT(RCC->APB1ENR,    RCC_APB1ENR_UART6EN);   // Enable UART6 clock (Alternate Function)
      SET_BIT(RCC->APB1RSTR,   RCC_APB1RSTR_UART6RST); // Reset UART6
      CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART6RST); // Release UART6 from reset
      irqNumber = UART6_IRQn;
      break;
    default:
      return FALSE;
  }

  Util_fillMemory(pUARTPort, sizeof(FullDuplexPort), 0x00);
  SET_BIT(pUARTConfig->CR1,   USART_CR1_RE);     // Enable RX
  SET_BIT(pUARTConfig->CR1,   USART_CR1_TE);     // Enable TX
  SET_BIT(pUARTConfig->CR1,   USART_CR1_RXNEIE); // Enable the RX not empty int
  CLEAR_BIT(pUARTConfig->CR1, USART_CR1_TXEIE);  // Disable the tx data reg empty int
  CLEAR_BIT(pUARTConfig->CR1, USART_CR1_TCIE);   // Disable the tx complete int
  pUARTConfig->CR2 &= 0xFFFF0000;                // Unnecessary, but for consistency
  pUARTConfig->CR3 &= 0xFFFF0000;                // No flow control
  pUARTConfig->BRR  = Uart_calculateBaudRateRegister(baud);
  SET_BIT(pUARTConfig->CR1, USART_CR1_UE);     // Enable UART

  if (enableInterrupt) // If requested, enable or disable the appropriate interrupt
    NVIC_EnableIRQ(irqNumber);
  else
    NVIC_DisableIRQ(irqNumber);
  
  return TRUE;
}

void UART5_IRQHandler(void)
{
  if (READ_BIT(UART5->SR, USART_SR_TXE)) // Transmit data register empty
  {
    if (sUART.U5.tx.numTx > 0)
    { // We still have more bytes to send
      UART5->DR = (sUART.U5.tx.txBuffer[sUART.U5.tx.bufIdx++] & 0x1FF);
      sUART.U5.tx.numTx--;
    }
    else
    { // Last byte is in the shift register
      CLEAR_BIT(UART5->CR1, USART_CR1_TXEIE); // Disable the tx data reg empty int
      SET_BIT(UART5->CR1, USART_SR_TC);       // Enable the transmit complete int
    }
//    CLEAR_BIT(UART5->SR, USART_SR_TXE); // Clear the interrupt flag
  }
  
  if (READ_BIT(UART5->SR, USART_SR_RXNE)) // Received data ready to be read
  {
    CLEAR_BIT(UART5->SR, USART_SR_RXNE); // Clear the interrupt flag 
  }
  
  if (READ_BIT(UART5->SR, USART_SR_TC)) // Last byte shifted out of the shift reg
  {
    CLEAR_BIT(UART5->CR1, USART_CR1_TCIE); // Disable the transmit complete int
    CLEAR_BIT(UART5->SR, USART_SR_TC);     // Clear the interrupt flag
  }
}

FullDuplexPort* UART_getPortPtr(SerialPort port)
{
  return &sUART.U5;
}

boolean UART_isTxBufIdle(SerialPort port)
{
  return sUART.U5.tx.numTx == 0;
}

/*****************************************************************************\
* FUNCTION    UART_sendData
* DESCRIPTION Writes character to the indicated serial port
* PARAMETERS    port: The port over which to transmit
              txByte: The byte to transmit
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
boolean UART_sendData(SerialPort port, uint16 numBytes)
{
  if (sUART.U5.tx.numTx == 0)
  { // UART is idle
    sUART.U5.tx.numTx  = numBytes;
    sUART.U5.tx.bufIdx = 0;
    SET_BIT(UART5->CR1, USART_CR1_TXEIE); // Enable the tx data reg empty int
    CLEAR_BIT(UART5->CR1, USART_SR_TC);   // Clear the transmission complete flag
    return TRUE;
  }
  else
    return FALSE; // UART is currently transmitting
}

/*****************************************************************************\
* FUNCTION    UART_putChar
* DESCRIPTION Writes character to the indicated serial port
* PARAMETERS    port: The port over which to transmit
              txByte: The byte to transmit
* RETURNS     Nothing
* NOTES       None
\*****************************************************************************/
boolean UART_putChar(SerialPort port, uint8 txByte)
{
  while (!(UART5->SR & USART_SR_TXE));  // Wait for transmit buffer empty
  UART5->DR = (txByte & 0x1FF);         // Load in the character to transmit
  return (txByte);
}

/*****************************************************************************\
* FUNCTION    UART_checkChar
* DESCRIPTION Checks to see if a character was received over the specified port
* PARAMETERS    port: The port to check
* RETURNS     TRUE if there is a byte available, FALSE otherwise
* NOTES       None
\*****************************************************************************/
boolean UART_checkChar(SerialPort port)
{
  if (UART5->SR & USART_SR_RXNE)
    return (TRUE);
  else
    return (FALSE);
}

/*****************************************************************************\
* FUNCTION    UART_getChar
* DESCRIPTION Retrieves a byte from the data register of the specified port
* PARAMETERS    port: The port to retrieve from
* RETURNS     The next available byte from the data register of the port
* NOTES       This function will block if a byte is not available
\*****************************************************************************/
uint8 UART_getChar(SerialPort port)
{
  while (!(UART5->SR & USART_SR_RXNE));
  return (UART5->DR & 0xFF);
}
