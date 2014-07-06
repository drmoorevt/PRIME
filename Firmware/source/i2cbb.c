#include "stm32f2xx.h"
#include "gpio.h"
#include "i2cbb.h"
#include "time.h"
#include "util.h"

#define FILE_ID I2CBB_C

#define SCL_PIN (GPIO_Pin_3)
#define SDA_PIN (GPIO_Pin_13)
#define SDA_DIR_HIH613X_I2C(x) do { GPIOB->MODER &= ~(GPIO_MODER_MODER0 << 26); \
                                    GPIOB->MODER |= (((uint32_t)x) << 26);      \
                                  } while (0)
#define SCL_DIR_HIH613X_I2C(x) do { GPIOD->MODER &= ~(GPIO_MODER_MODER0 << 6); \
                                    GPIOD->MODER |= (((uint32_t)x) << 6);      \
                                  } while (0)

#define SDA_HIGH_HIH613X_I2C() do { GPIOB->BSRRL |= 0x00002000;         \
                                    SDA_DIR_HIH613X_I2C(GPIO_Mode_IN);  } while (0)
#define SDA_LOW_HIH613X_I2C()  do { GPIOB->BSRRH |= 0x00002000;         \
                                    SDA_DIR_HIH613X_I2C(GPIO_Mode_OUT); } while (0)

#define SCL_HIGH_HIH613X_I2C() do { GPIOB->BSRRL |= 0x00000200;         \
                                    SCL_DIR_HIH613X_I2C(GPIO_Mode_IN);  } while (0)
#define SCL_LOW_HIH613X_I2C()  do { GPIOB->BSRRH |= 0x00000200;         \
                                    SCL_DIR_HIH613X_I2C(GPIO_Mode_OUT); } while (0)

static struct
{
  uint32 clockFreqKhz;
  uint32 clockTimeUs;
  uint32 setupTimeSDA;
} sI2CBB;

static void    I2CBB_sendStart(void);
static void    I2CBB_sendStop(void);
static boolean I2CBB_getAck(void);
static boolean I2CBB_sendByte(uint8 byte);
static void    I2CBB_sendAckNack(boolean ack);
static uint8   I2CBB_getByte(boolean ack);

/**************************************************************************************************\
* FUNCTION    I2CBB_init
* DESCRIPTION Initializes the I2CBB (bit-banged) module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void I2CBB_init(void)
{
  Util_fillMemory(&sI2CBB, sizeof(sI2CBB), 0x00);
  I2CBB_setup(FALSE, I2CBB_CLOCK_RATE_101562);
}

/**************************************************************************************************\
* FUNCTION    I2CBB_setup
* DESCRIPTION Enables or Disables the pins required to operate the HIH613X sensor
* PARAMETERS  state: If TRUE, required pins will be output. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
\**************************************************************************************************/
boolean I2CBB_setup(boolean state, I2CBBClockRate rate)
{
  // Initialize the SCL and SDA lines (Note open drain on SDA and push pull on SCL)
  GPIO_InitTypeDef SCLI2CBBCtrl = {SCL_PIN, GPIO_Mode_IN, GPIO_Speed_25MHz, GPIO_OType_OD,
                                                     GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef SDAI2CBBCtrl = {SDA_PIN, GPIO_Mode_IN, GPIO_Speed_25MHz, GPIO_OType_OD,
                                                     GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  
  // @DRM: Use the input clock rate here...
  sI2CBB.setupTimeSDA = (1);
  sI2CBB.clockFreqKhz = (100);
  sI2CBB.clockTimeUs  = (1000) / sI2CBB.clockFreqKhz;
  
  GPIO_configurePins(GPIOD, &SCLI2CBBCtrl);
  GPIO_configurePins(GPIOB, &SDAI2CBBCtrl);
  GPIO_setPortClock(GPIOB, TRUE);
  I2CBB_sendStop();

  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    I2CBB_sendStart
* DESCRIPTION Sends a start signal to the I2C device. SCL low then SDA low
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void I2CBB_sendStart(void)
{
  SDA_LOW_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
  SCL_LOW_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
}

/**************************************************************************************************\
* FUNCTION    I2CBB_sendStop
* DESCRIPTION Sends a stop signal to the I2C device. SCL High, then release SDA (floats high)
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void I2CBB_sendStop(void)
{
  SCL_HIGH_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
  SDA_HIGH_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
}

/**************************************************************************************************\
* FUNCTION    I2CBB_getAck
* DESCRIPTION Gets an ACK signal from the SDA pin
* PARAMETERS  None
* RETURNS     TRUE if the SDA pin is low (ACK), FALSE if the SDA pin is high (NAK)
\**************************************************************************************************/
static boolean I2CBB_getAck(void)
{
  boolean ack;
  SDA_DIR_HIH613X_I2C(GPIO_Mode_IN);
  SCL_HIGH_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
  ack = ((GPIOB->IDR & GPIO_IDR_IDR_13) > 0);
  SCL_LOW_HIH613X_I2C();
  return ack;
}

/**************************************************************************************************\
* FUNCTION    I2CBB_sendByte
* DESCRIPTION Clocks out a byte over I2CBB
* PARAMETERS  byte: The byte to send out
* RETURNS     TRUE if the byte was ACKed, FALSE if the byte was NACKed
\**************************************************************************************************/
static boolean I2CBB_sendByte(uint8 byte)
{
  uint8 count;
  for (count = 0; count < 8; count++) // Clock out the bits of command, MSB first
  {
    Time_delay(sI2CBB.clockTimeUs);
    
    if (byte & 0x80)
      SDA_HIGH_HIH613X_I2C();
    else
      SDA_LOW_HIH613X_I2C();
    byte <<= 1;
    
    Time_delay(sI2CBB.setupTimeSDA);
    SCL_HIGH_HIH613X_I2C();
    
    Time_delay(sI2CBB.clockTimeUs);
    SCL_LOW_HIH613X_I2C();
  }
  return I2CBB_getAck(); // Switch the SDA direction and receive the ACK signal from the device
}

/**************************************************************************************************\
* FUNCTION    I2CBB_sendAckNack
* DESCRIPTION Sends an ACK or NACK out of the SDA pin
* PARAMETERS  ack: If false, sends an ACK (output low), otherwise sends a NAK (output high)
* RETURNS     None
\**************************************************************************************************/
static void I2CBB_sendAckNack(boolean ack)
{
  if (ack)
    SDA_LOW_HIH613X_I2C();
  else
    SDA_HIGH_HIH613X_I2C();
  Time_delay(sI2CBB.setupTimeSDA);
  SCL_HIGH_HIH613X_I2C();
  Time_delay(sI2CBB.clockTimeUs);
  SCL_LOW_HIH613X_I2C();
  SDA_HIGH_HIH613X_I2C(); // Turn it around into an input...
}

/**************************************************************************************************\
* FUNCTION    I2CBB_getByte
* DESCRIPTION Clocks in a byte over I2CBB
* PARAMETERS  ack: indicates whether to send an ACK or NACK after receiving the byte
* RETURNS     The received byte
\**************************************************************************************************/
static uint8 I2CBB_getByte(boolean ack)
{
  uint8 count, byte;
  for (count = 8, byte = 0; count != 0; count--)
  {
    Time_delay(sI2CBB.clockTimeUs);
    SCL_HIGH_HIH613X_I2C();
    Time_delay(sI2CBB.setupTimeSDA);
    byte <<= 1;
    if (GPIOB->IDR & GPIO_IDR_IDR_13)
      byte |= 1;
    SCL_LOW_HIH613X_I2C();
  }
  I2CBB_sendAckNack(ack);
  return byte;
}

/**************************************************************************************************\
* FUNCTION    I2CBB_readData
* DESCRIPTION Sends and command and reads the response at the specified frequency on portB
* PARAMETERS  command: The command to send, preceding the response
*             pResponse: A buffer in which to store the response (can be NULL if respLength == 0)
*             respLength: The number of bytes to retrieve as response
* RETURNS     TRUE if the command was acknowledged
* NOTES       1) Currently only bit bangs pins 9 and 13 of PORTB
*             2) The configuration of GPIOB is stored on entry and restored on exit.
*                NOTE THAT IF INTERRUPTS CHANGE THE PORT CONFIGURATION THAT IT WILL BE RESTORED
\**************************************************************************************************/
boolean I2CBB_readData(uint8 command, uint8 *pResponse, uint8 respLength)
{
  boolean ack;

  I2CBB_sendStart();
  ack = I2CBB_sendByte(command);
  while (respLength--) // will fall through if no response is requested
    *pResponse++ = I2CBB_getByte(respLength); // Send NAK on last byte
  I2CBB_sendStop();

  return ack;
}
