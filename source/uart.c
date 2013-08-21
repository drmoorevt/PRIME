#include "stm32f2xx.h"
#include "util.h"
#include "uart.h"

#define FILE_ID UART_C

struct
{
  FullDuplexPort U5;
} sUART;

/*************************************************************************************************\
* FUNCTION    UART_init
* DESCRIPTION Initializes the selected port to its default configuration
* PARAMETERS  port: The UART port to initialize and configure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
boolean UART_init(SerialPort port)
{
  Util_fillMemory((uint8*)&sUART, sizeof(sUART), 0x00);
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_UART5EN);      // Enable UART5 clock (Alternate Function)
  SET_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART5RST);   // Reset UART5
  CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_UART5RST); // Release UART5 from reset
  
  // 115200 baud, 8 data bits, 1 stop bit, no flow control
  SET_BIT(UART5->CR1, USART_CR1_RE);     // Enable RX
  SET_BIT(UART5->CR1, USART_CR1_TE);     // Enable TX
  SET_BIT(UART5->CR1, USART_CR1_RXNEIE); // Enable the RX not empty int
  CLEAR_BIT(UART5->CR1, USART_CR1_TXEIE);// Disable the tx data reg empty int
  CLEAR_BIT(UART5->CR1, USART_CR1_TCIE); // Disable the tx complete int
  UART5->CR2 &= 0xFFFF0000;              // Unnecessary, but for consistency
  UART5->CR3 &= 0xFFFF0000;              // No flow control
  UART5->BRR  = 0x0104;                  // 12bit Divider with 4 bit fractional divider,
                                         // UART5 clocked by Pclk1 115.2 = 16.25
  SET_BIT(UART5->CR1, USART_CR1_UE);     // Enable UART
  NVIC_EnableIRQ(UART5_IRQn);            // Enable UART interrupts
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
