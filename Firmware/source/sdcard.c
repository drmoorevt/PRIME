#include "stm32f2xx.h"
#include "analog.h"
#include "gpio.h"
#include "types.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "sdcard.h"

#pragma anon_unions

#define FILE_ID SDCARD_C

#define WRITEPAGESIZE_SF ((uint16)256)
#define SF_NUM_RETRIES 3
#define ADDRBYTES_SF 3
#define SD_MAX_WAIT_BYTES 10;

#define SELECT_CHIP_SD()    do { GPIOB->BSRRH |= 0x00000010; } while (0)
#define DESELECT_CHIP_SD()  do { GPIOB->BSRRL |= 0x00000010; } while (0)

typedef enum
{
  GO_IDLE_STATE        = (0x40 + 0),  // CMD0
  SEND_OP_COND_MMC     = (0x40 + 1),  // CMD1
  SEND_OP_COND_SDC     = (0xC0 + 41), // ACMD41
  SEND_IF_COND         = (0x40 + 8),  // CMD8
  SEND_CSD             = (0x40 + 9),  // CMD9
  SEND_CID             = (0x40 + 10), // CMD10
  STOP_TRANSMISSION    = (0x40 + 12), // CMD12
  SD_STATUS_SDC        = (0xC0 + 13), // ACMD13
  SET_BLOCKLEN         = (0x40 + 16), // CMD16
  READ_SINGLE_BLOCK    = (0x40 + 17), // CMD17
  READ_MULTIPLE_BLOCK  = (0x40 + 18), // CMD18
  SET_BLOCK_COUNT      = (0x40 + 23), // CMD23
  SET_WR_BLK_ERASE_CNT = (0xC0 + 23), // ACMD23
  WRITE_BLOCK          = (0x40 + 24), // CMD24
  WRITE_MULTIPLE_BLOCK = (0x40 + 25), // CMD25
  APP_CMD              = (0x40 + 55), // CMD55
  READ_OCR             = (0x40 + 58)  // CMD58
} SDCommand;

typedef enum
{
  UNDEFINED   = 0x0,
  VOLTAGE_OK  = 0x1,
  LOW_VOLTAGE = 0x2,
  RESERVED0   = 0x4,
  RESERVED1   = 0x8
} VoltageStatus;

__packed typedef struct
{
  uint16 reserved;
  uint8  voltageOk;  // Actually 4 bits of reserved zeros in upper nibble, only assign VoltageStatus
  uint8  checkPattern;
} SendIFCondArg;

__packed typedef struct
{
  uint8  reserved1  : 8;
  uint16 ocr        : 16;
  uint8  s18r       : 1;
  uint8  reserved0  : 3;
  uint8  xpc        : 1;
  uint8  fb         : 1;
  uint8  hcs        : 1;
  uint8  busy       : 1;
} SendOpCondArg;

__packed typedef union
{
  uint32 reference;
  SendIFCondArg  sendIFCondArg;
  SendOpCondArg  sendOpCondArg;
} SDCommandArgument;

__packed typedef struct
{
  uint8             cmd;
  SDCommandArgument arg;
  uint8             crc;
} SDCommandRequest;

__packed typedef struct
{
  uint8 idleState     : 1;
  uint8 eraseReset    : 1;
  uint8 illegalCmd    : 1;
  uint8 cmdCRCError   : 1;
  uint8 eraseSeqError : 1;
  uint8 addrError     : 1;
  uint8 paramError    : 1;
  uint8 filler        : 1;
} SDCommandResponseR1;

__packed typedef struct
{
  uint8 cardLocked    : 1;
  uint8 wpEraseSkip   : 1;
  uint8 unknownError  : 1;
  uint8 ccError       : 1;
  uint8 cardECCFailed : 1;
  uint8 wpViolation   : 1;
  uint8 badEraseParam : 1;
  uint8 outOfRange    : 1;
} SDCommandResponseR2;

__packed typedef struct
{
  uint32 echoBack   : 8;
  uint32 voltageOk  : 4;
  uint32 reserved   : 16;
  uint32 cmdVersion : 4;
} SendIfCondRespArg;

__packed typedef struct
{
  uint32 reserved0  : 8;
  uint32 ocr        : 16;
  uint32 s18a       : 1;
  uint32 reserved1  : 4;
  uint32 ush2       : 1;
  uint32 ccs        : 1;
  uint32 busy       : 1;
} SendOpCondRespArg;

__packed typedef struct
{
  uint8 reserved2      : 8;
  uint8 reserved1      : 7;
  uint8 voltage2728    : 1;
  uint8 voltage2829    : 1;
  uint8 voltage2930    : 1;
  uint8 voltage3031    : 1;
  uint8 voltage3132    : 1;
  uint8 voltage3233    : 1;
  uint8 voltage3334    : 1;
  uint8 voltage3435    : 1;
  uint8 voltage3536    : 1;
  uint8 switchingOk    : 1;
  uint8 reserved0      : 4;
  uint8 uhs2CardStatus : 1;
  uint8 capacityStatus : 1;
  uint8 powerUpStatus  : 1;
} ReadOCRRespArg;

__packed typedef struct
{
  SDCommandResponseR1 stdResp;
  __packed union
  {
    uint32 reference;
    SendIfCondRespArg ifCondResp;
    SendOpCondRespArg opCondResp;
    ReadOCRRespArg    ocrResp;
  } arg;
} SDCommandResponse;

static struct
{
  SDCardState state;
} sSDCard;

/**************************************************************************************************\
* FUNCTION    SDCard_init
* DESCRIPTION Initializes the SDCard module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SDCard_init(void)
{
  const uint16 sfCtrlPins = (GPIO_Pin_4);

  // Initialize the chip select line
  GPIO_InitTypeDef sdCtrlPortB = {sfCtrlPins, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                              GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &sdCtrlPortB);
  DESELECT_CHIP_SD();

  Util_fillMemory((uint8*)&sSDCard, sizeof(sSDCard), 0x00);
  sSDCard.state = SD_CARD_IDLE;
}

/**************************************************************************************************\
* FUNCTION    SDCard_sendCommand
* DESCRIPTION txBuf: A pointer to the data to be transmitted. Can be NULL.
*             txLen: The number of bytes to be transmitted.
*             rxBuf: A pointer to the receive buffer. Can be NULL.
*             rxLen: The number of bytes to receive.
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static SDCommandResponse SDCard_sendCommand(SDCommand cmd, uint32 arg)
{
  SDCommandResponse resp;
  uint8 cmdTx;
  uint8 crc = 0;
  uint16 waitBytes = SD_MAX_WAIT_BYTES;

  if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
    cmd &= 0x7F;
    resp = SDCard_sendCommand(APP_CMD, 0);
    if (*(uint8 *)&resp.stdResp > 1)
      return resp;
  }

  Util_swap32(&arg);

  cmdTx = cmd;
  crc   = (Util_calcCRC7(crc, &cmdTx, 1));
  crc   = (Util_calcCRC7(crc, (uint8 *)&arg, 4) << 1) | 0x01;

  SELECT_CHIP_SD();                   // Select the SDCard
  SPI_write(&cmdTx, 1);
  SPI_write((uint8 *)&arg, 4);

  if ((cmd == GO_IDLE_STATE)|| (cmd == SEND_IF_COND))
    SPI_write(&crc, 1);

  do
  {
    SPI_read((uint8 *)&resp.stdResp, 1);             // Continuous read until the first byte is non-zero
  } while ((*(uint8 *)&resp.stdResp == 0xFF) && (waitBytes--));
  SPI_read((uint8 *)&resp.arg, 4);

  DESELECT_CHIP_SD();                 // Deselect the SDCard

  Util_swap32((uint32 *)&resp.arg);

  return resp;
}

/**************************************************************************************************\
* FUNCTION    SDCard_setup
* DESCRIPTION Puts the SDCard into SPI mode
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
boolean SDCard_setup(void)
{
  uint8 trys;
  boolean success = TRUE;
  SDCommandResponse goIdleResp, sendIfCondResp, sendOpCondResp, readOcrResp;
  uint8 OP_SD_SPI_MODE[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  Analog_setDomain(ANALOG_DOMAIN, TRUE);   // Enable analog domain
  Analog_setDomain(IO_DOMAIN,     TRUE);   // Enable I/O domain
  Analog_setDomain(EEPROM_DOMAIN, TRUE);   // Enable SPI domain
  Analog_adjustDomain(EEPROM_DOMAIN, 0.65); // Set domain voltage to nominal (3.25V)
  Time_delay(1000); // Wait 1000ms for domains to settle

  // Send the SPI mode command with the chip deselected ... prime the pump if you will...
  SPI_write(OP_SD_SPI_MODE, sizeof(OP_SD_SPI_MODE));

  goIdleResp = SDCard_sendCommand(GO_IDLE_STATE, 0x00000000);
  sendIfCondResp = SDCard_sendCommand(SEND_IF_COND, 0x000001AA);

  trys = 0;
  do
  {
    sendOpCondResp = SDCard_sendCommand(SEND_OP_COND_SDC, 0x40000000);
  } while ((*(uint8*)&sendOpCondResp.stdResp > 0) && (trys++ < 254));

  readOcrResp = SDCard_sendCommand(READ_OCR, 0x00000000);

  // Set Block length CMD(16) to 512bytes for reads (CMD17 single, CMD18 multi)

  // CMD24 single write, CMD25 multi write

  return success;
}

/**************************************************************************************************\
* FUNCTION    SDCard_transceive
* DESCRIPTION txBuf: A pointer to the data to be transmitted. Can be NULL.
*             txLen: The number of bytes to be transmitted.
*             rxBuf: A pointer to the receive buffer. Can be NULL.
*             rxLen: The number of bytes to receive.
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
static boolean SDCard_transceive(uint8 *txBuf, uint16 txLen, uint8 *rxBuf, uint16 rxLen)
{
  uint16 waitBytes = SD_MAX_WAIT_BYTES;

  SELECT_CHIP_SD();                   // Select the SDCard

  if ((txBuf != NULL) && (txLen > 0))
    SPI_write(txBuf, txLen);  // If a command is present, transmit it

  if ((rxBuf != NULL) && (rxLen > 0)) // If a response is requested, begin receiving it
  {
    do
    {
      SPI_read(rxBuf, 1);             // Continuous read until the first byte is non-zero
    } while ((*rxBuf == 0xFF) && (waitBytes--));
    SPI_read(rxBuf+1, rxLen-1);       // Read the rest of the bytes into the rxBuffer
  }

  DESELECT_CHIP_SD();                 // Deselect the SDCard

  return (waitBytes > 0);
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
  return 0;
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

}

/*****************************************************************************\
* FUNCTION    SDCard_writeFlash
* DESCRIPTION Writes a buffer to Serial Flash
* PARAMETERS  pSrc - pointer to source RAM buffer
*             pDest - pointer to destination in SDCard
*             length - number of bytes to write
* RETURNS     true if the write succeeds
\*****************************************************************************/
boolean SDCard_writeFlash(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  return (boolean)FALSE;
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
