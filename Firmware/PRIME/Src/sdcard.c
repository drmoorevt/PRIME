#include "analog.h"
#include "crc.h"
#include "powercon.h"
#include "spi.h"
#include "time.h"
#include "util.h"
#include "sdcard.h"
#include "stm32f4xx_hal.h"

#define FILE_ID SDCARD_C

#define SD_MAX_WAIT_PRE_WRITE_BYTES (10)
#define SD_MAX_WAIT_BUS_BYTES       (65535)
#define SD_MAX_WAIT_INIT_BYTES      (65535) // (4095)
#define SD_MAX_WAIT_WRITE_BYTES     (65536) // (4095)
#define SD_MAX_WAIT_RESP_BYTES      (65536) // (4096) // (255)

// Can remove the wait by configuring the pins as push-pull, but risk leakage into the domain
#define SELECT_CHIP_SD()   do {                                                              \
                                 HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET); \
                                 Time_delay(1);                                               \
                               } while (0)

#define DESELECT_CHIP_SD() do {                                                              \
                                 HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); \
                                 Time_delay(1);                                               \
                               } while (0)

#define START_SINGLE_BLOCK_TOKEN    (0xFE)
#define START_MULTIPLE_BLOCK_TOKEN  (0xFC)
#define STOP_MULTIPLE_BLOCK_TOKEN   (0xFD)

#define DEFAULT_BLOCK_LENGTH (512)

#define SD_HIGH_SPEED_VMIN (3.3)
#define SD_LOW_SPEED_VMIN (2.7)

typedef enum
{
  GO_IDLE_STATE        = (0x40 + 0),  // CMD0
  SEND_OP_COND_MMC     = (0x40 + 1),  // CMD1
  SEND_OP_COND_SDC     = (0xC0 + 41), // ACMD41
  SEND_IF_COND         = (0x40 + 8),  // CMD8
  SEND_CSD             = (0x40 + 9),  // CMD9
  SEND_CID             = (0x40 + 10), // CMD10
  STOP_TRANSMISSION    = (0x40 + 12), // CMD12
  CARD_STATUS          = (0x40 + 13), // CMD13
  SD_STATUS_SDC        = (0xC0 + 13), // ACMD13
  SET_BLOCKLEN         = (0x40 + 16), // CMD16
  READ_SINGLE_BLOCK    = (0x40 + 17), // CMD17
  READ_MULTIPLE_BLOCK  = (0x40 + 18), // CMD18
  SET_BLOCK_COUNT      = (0x40 + 23), // CMD23
  SET_WR_BLK_ERASE_CNT = (0xC0 + 23), // ACMD23
  WRITE_BLOCK          = (0x40 + 24), // CMD24
  WRITE_MULTIPLE_BLOCK = (0x40 + 25), // CMD25
  LOCK_UNLOCK          = (0x40 + 42), // CMD42
  APP_CMD              = (0x40 + 55), // CMD55
  READ_OCR             = (0x40 + 58)  // CMD58
} SDCommand;

typedef enum
{
  SDCARD_RESPONSE_OK      = 0,
  SDCARD_COMMAND_ERROR    = 1,
  SDCARD_TIMEOUT          = 2,
  SDCARD_ERROR            = 3,
  SDCARD_BAD_CRC          = 4,
  SDCARD_DATA_ERROR       = 5,
  SDCARD_CONTROLLER_ERROR = 6,
  SDCARD_ECC_FAILED       = 7,
  SDCARD_OUT_OF_RANGE     = 8,
  SDCARD_WRITE_ERROR      = 9
} SDCommandResult;

typedef struct
{
  uint8 error               : 1;
  uint8 cardControllerError : 1;
  uint8 cardECCFailed       : 1;
  uint8 outOfRange          : 1;
  uint8 filler              : 4;
} SDDataErrorToken;

typedef enum
{
  UNDEFINED   = 0x0,
  VOLTAGE_OK  = 0x1,
  LOW_VOLTAGE = 0x2,
  RESERVED0   = 0x4,
  RESERVED1   = 0x8
} VoltageStatus;

/***** Bitfields are allocated so that the first field specified occupies the lowest-addressed bits of the word *****/

/***** Note that structures are packed in MSB to LSB in descending order *****/

typedef struct
{
  uint16 reserved;
  uint8  voltageOk;  // Actually 4 bits of reserved zeros in upper nibble, only assign VoltageStatus
  uint8  checkPattern;
} SendIFCondArg;

typedef struct
{
  uint32  reserved1  : 8;
  uint32  ocr        : 16;
  uint32  s18r       : 1;
  uint32  reserved0  : 3;
  uint32  xpc        : 1;
  uint32  fb         : 1;
  uint32  hcs        : 1;
  uint32  busy       : 1;
} SendOpCondArg;

typedef union
{
  uint32 reference;
  SendIFCondArg  sendIFCondArg;
  SendOpCondArg  sendOpCondArg;
} SDCommandArgument;

typedef struct
{
  uint8             cmd;
  SDCommandArgument arg;
  uint8             crc;
} SDCommandRequest;

typedef struct
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

typedef struct
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

typedef struct
{
  uint32 reserved2      : 8;
  uint32 reserved1      : 7;
  uint32 voltage2728    : 1;
  uint32 voltage2829    : 1;
  uint32 voltage2930    : 1;
  uint32 voltage3031    : 1;
  uint32 voltage3132    : 1;
  uint32 voltage3233    : 1;
  uint32 voltage3334    : 1;
  uint32 voltage3435    : 1;
  uint32 voltage3536    : 1;
  uint32 switchingOk    : 1;
  uint32 reserved0      : 4;
  uint32 uhs2CardStatus : 1;
  uint32 capacityStatus : 1;
  uint32 powerUpStatus  : 1;
} SDCommandResponseR3;

typedef struct
{
  uint32 echoBack   : 8;
  uint32 voltageOk  : 4;
  uint32 reserved   : 16;
  uint32 cmdVersion : 4;
} SDCommandResponseR7;

typedef struct
{
  uint32 reserved0  : 8;
  uint32 ocr        : 16;
  uint32 s18a       : 1;
  uint32 reserved1  : 4;
  uint32 ush2       : 1;
  uint32 ccs        : 1;
  uint32 busy       : 1;
} SendOpCondRespArg;

typedef struct
{
  SDCommandResponseR1 stdResp;
  union
  {
    uint32 reference;
    SDCommandResponseR2 statusResp;
    SDCommandResponseR2 ifCondResp;
    SendOpCondRespArg   opCondResp;
    SDCommandResponseR3 ocrResp;
  } arg;
} SDCommandResponse;

typedef struct
{
  uint32 dataBusWidth  : 1;
  uint32 securedMode   : 1;
  uint32 securityRsvd  : 7;
  uint32 reserved0     : 6;
  uint32 sdCardType    : 16;
  uint32 sizeOfProtect : 32;
  uint32 speedClass    : 8;
  uint32 perfMove      : 8;
  uint32 auSize        : 4;
  uint32 reserved1     : 4;
  uint32 eraseSize     : 16;
  uint32 eraseTimeout  : 6;
  uint32 eraseOffset   : 2;
  uint32 uhsSpeedGrade : 4;
  uint32 uhsAUSize     : 4;
  uint8  reserved2[10];
  uint8  reservedMfg[39];
} SDStatusRespBlock;

typedef __packed struct
{
  uint8  mfgId;
  char   appId[2];
  char   name[5];
  uint8  revision;
  uint32 serial;
  
  uint16 mfgYearH : 4;
  uint16 reserved : 4;
  uint16 mfgMonth : 4;
  uint16 mfgYearL : 4;
  
  uint8  fillOne  : 1; // lsb --> msb
  uint8  crc7     : 7;
} CardIdReg; // CID Reg

typedef __packed struct
{
  uint8  csdType;
  uint8  taac;
  uint8  nsac;
  uint8  tranSpeed;
  
  uint64 writeProtectGrpSize : 7;
  uint64 sectorSize          : 7;
  uint64 eraseSingleBlockEn  : 1;
  uint64 deviceSizeMultiple  : 3;
  uint64 maxWriteCurrVddMax  : 3;
  uint64 maxWriteCurrVddMin  : 3;
  uint64 maxReadCurrVddMax   : 3;
  uint64 maxReadCurrVddMin   : 3;
  uint64 deviceSize          : 12;
  uint64 reserved0           : 2;
  uint64 dsrImp              : 1;
  uint64 readBlockMisAlign   : 1;
  uint64 writeBlockMisAlign  : 1;
  uint64 readBlockPartial    : 1;
  uint64 readBlockLen        : 4;
  uint64 cardComClass        : 12;
  
  uint32 fillOne             : 1;
  uint32 crc7                : 7;
  uint32 reserved2           : 2;
  uint32 fileFormat          : 2;
  uint32 tempWriteProtect    : 1;
  uint32 permWriteProtect    : 1;
  uint32 copyFlag            : 1;
  uint32 fileFormatGroup     : 1;
  uint32 reserved            : 5;
  uint32 writeBlockPartialEn : 1;
  uint32 writeBlockLength    : 4;
  uint32 writeSpeedFactor    : 3;
  uint32 reserved1           : 2;
  uint32 writeProtectGrpEn   : 1;
} CardStatusReg;

typedef struct
{
  union
  {
    uint8 reference[DEFAULT_BLOCK_LENGTH];
    SDStatusRespBlock sdStatus;
  } arg;
} SDResponseBlock;

// Power profile voltage definitions, in SDCardPowerProfile / SDCardState order
static const double SDCARD_POWER_PROFILES[SDCARD_PROFILE_MAX][SDCARD_STATE_MAX] =
{ // Idle, Setup, Ready, Reading, Writing, Waiting, Verifying
  {3.3, 3.3, 3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile: 3.3VISRW
  {3.0, 3.3, 3.0, 3.3, 3.3, 3.0, 3.3},  // 3.0VISRW
  {2.7, 3.3, 2.7, 3.3, 3.3, 2.7, 3.3},  // 2.7VISRW
  {2.4, 3.3, 2.4, 3.3, 3.3, 2.4, 3.3},  // 2.4VISRW
  {2.1, 3.3, 2.1, 3.3, 3.3, 2.1, 3.3}   // 2.1VISRW
};

static struct
{
  SDCardState     state;
  double          vDomain[SDCARD_STATE_MAX]; // The domain voltage for each state
  SDResponseBlock respBlock;
  CardIdReg       sdCardId;
  CardStatusReg   sdCardStatus;
  uint32          capacityMult;
  uint32          cardCapacity;

  uint32          blockLen;
  uint32          cardBlockLen;
  uint32          cmdWaitClocks;
  uint32          writeWaitClocks;
  uint32          preReadWaitClocks;
  uint32          postReadWaitClocks;
  uint32          preWriteWaitClocks;
  uint32          postWriteWaitClocks;
  boolean         isSDHC;
  boolean         isInitialized;
} sSDCard;

static SDCommandResult SDCard_readBlock(SDCommand cmd, uint32 arg);

SDCardTiming SDCard_getLastTiming(void)
{
  SDCardTiming timing =
  {
    .cmdWaitClocks       = sSDCard.cmdWaitClocks,
    .writeWaitClocks     = sSDCard.writeWaitClocks,
    .preReadWaitClocks   = sSDCard.preReadWaitClocks,
    .postReadWaitClocks  = sSDCard.postReadWaitClocks,
    .preWriteWaitClocks  = sSDCard.preWriteWaitClocks,
    .postWriteWaitClocks = sSDCard.postWriteWaitClocks
  };
  return timing;
}

/**************************************************************************************************\
* FUNCTION    SDCard_init
* DESCRIPTION Initializes the SDCard module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SDCard_init(void)
{
  Util_fillMemory((uint8*)&sSDCard, sizeof(sSDCard), 0x00);
  sSDCard.blockLen = DEFAULT_BLOCK_LENGTH;
  SDCard_setPowerProfile(SDCARD_PROFILE_STANDARD);  // Set all states to 3.3v
  SDCard_setup(FALSE);
  sSDCard.state = SDCARD_STATE_IDLE;
  sSDCard.isInitialized = TRUE;
}

/**************************************************************************************************\
* FUNCTION    SDCard_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the SDCard
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state if the SPI pins
\**************************************************************************************************/
boolean SDCard_setup(boolean state)
{
  // Initialize the SDCard chip select line
  DESELECT_CHIP_SD();
  
  // Set up the SPI transaction with respect to domain voltage
  if (sSDCard.vDomain[sSDCard.state] >= SD_HIGH_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_00703125, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else if (sSDCard.vDomain[sSDCard.state] >= SD_LOW_SPEED_VMIN)
    SPI_setup(state, SPI_CLOCK_RATE_00703125, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL);
  else
    SPI_setup(state, SPI_CLOCK_RATE_01406250, SPI_PHASE_1EDGE, SPI_POLARITY_LOW, SPI_MODE_NORMAL); // Voltage too low, attempt at very low speed
  
  return (sSDCard.vDomain[sSDCard.state] >= SD_LOW_SPEED_VMIN);
}

/**************************************************************************************************\
* FUNCTION    SDCard_getStatePointer
* DESCRIPTION Returns the current state of the SDCard
* PARAMETERS  None
* RETURNS     The current state of SDCard
\**************************************************************************************************/
uint32_t *SDCard_getStatePointer(void)
{
  return (uint32_t *)&sSDCard.state;
}

/**************************************************************************************************\
* FUNCTION    SDCard_getStateVoltage
* DESCRIPTION Returns the ideal voltage of the current state (as dictated by the current profile)
* PARAMETERS  None
* RETURNS     The ideal state voltage
\**************************************************************************************************/
double SDCard_getStateVoltage(void)
{
  return sSDCard.vDomain[sSDCard.state];
}

/**************************************************************************************************\
* FUNCTION    SDCard_setState
* DESCRIPTION Sets the internal state of the SDCard and applies the voltage of the associated state
* PARAMETERS  None
* RETURNS     None
\**************************************************************************************************/
static void SDCard_setState(SDCardState state)
{
  if (sSDCard.isInitialized != TRUE)
    return;  // Must run initialization before we risk changing the domain voltage
  
  VoltageDomain curDomain;
  switch (state)
  {
    case SDCARD_STATE_IDLE:      curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for idle
    case SDCARD_STATE_SETUP:     curDomain = VOLTAGE_DOMAIN_2; break;  // MCU domain for reading
    case SDCARD_STATE_READY:     curDomain = VOLTAGE_DOMAIN_2; break;  // MCU domain for writing
    case SDCARD_STATE_READING:   curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for reading
    case SDCARD_STATE_WRITING:   curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for writing
    case SDCARD_STATE_WAITING:   curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for waiting
    case SDCARD_STATE_VERIFYING: curDomain = VOLTAGE_DOMAIN_2; break;  // Modular domain for verifying
    default:                     curDomain = VOLTAGE_DOMAIN_2; break;  // Error...
  }
  
  sSDCard.state = state;
  PowerCon_setDeviceDomain(DEVICE_SDCARD, curDomain);
  PowerCon_setDomainVoltage(curDomain, sSDCard.vDomain[state]);  // Set the domain voltage
}

/**************************************************************************************************\
* FUNCTION    SDCard_setPowerState
* DESCRIPTION Sets the buck feedback voltage for a particular state of the SDCard
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SDCard_setPowerState(SDCardState state, double vDomain)
{
  if (state >= SDCARD_STATE_MAX)
    return FALSE;
  else if (vDomain > 3.3)
    return FALSE;
  else
    sSDCard.vDomain[state] = vDomain;
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SDCard_setPowerProfile
* DESCRIPTION Sets all power states of the SDCard to the specified profile
* PARAMETERS  None
* RETURNS     TRUE if the voltage can be set for the state, false otherwise
\**************************************************************************************************/
boolean SDCard_setPowerProfile(SDCardPowerProfile profile)
{
  uint32 state;
  if (profile >= SDCARD_PROFILE_MAX)
    return FALSE;  // Invalid profile, inform the caller
  for (state = 0; state < SDCARD_STATE_MAX; state++)
    sSDCard.vDomain[state] = SDCARD_POWER_PROFILES[profile][state];
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SDCard_notifyVoltageChange
* DESCRIPTION Called when any other task changes the voltage of the domain
* PARAMETERS  newVoltage: The voltage that the domain is now experiencing
* RETURNS     Nothing
\**************************************************************************************************/
void SDCard_notifyVoltageChange(double newVoltage)
{
  if (newVoltage < 2.3)
    sSDCard.state = SDCARD_STATE_IDLE;
}

/**************************************************************************************************\
* FUNCTION    SDCard_waitReady
* DESCRIPTION Waits for the specified byte for a specified # of SPI_Reads
* PARAMETERS      waitByte - When this byte appears on the SPI bus, the function will return
*             maxWaitBytes - The maximum number of SPI bytes to check before returning
* RETURNS     The number of bytes elapsed before finding the waitByte
\**************************************************************************************************/
static uint32 SDCard_waitReady(uint8 waitByte, uint32 maxWaitBytes)
{
  uint32 clocks = 0;
  uint8 bus = ~waitByte;
  while ((bus != waitByte) && ((clocks++) < maxWaitBytes))
    SPI_read(&bus, 1);
  return clocks;
}

/**************************************************************************************************\
* FUNCTION    SDCard_sendCommand
* DESCRIPTION Sends one of the SDCommands with its associated argument and returns the R1 response
* PARAMETERS        cmd - One of the SDCommands
*                   arg - The argument (little-endian) of the SDCommand cmd
*             waitBytes - The number of bytes to wait for a valid R1 response from the SDCard, 0
*                         indicates no response
* RETURNS     The R1 response to the command from the SDCard
\**************************************************************************************************/
static SDCommandResponseR1 SDCard_sendCommand(SDCommand cmd, uint32 arg, uint32 waitBytes)
{
  SDCommandResponseR1 resp;
  uint8 cmdTx[6];

  if (cmd & 0x80)  /* ACMD is the command sequence of CMD55 */
  {
    cmd &= 0x7F;
    resp = SDCard_sendCommand(APP_CMD, 0x00000000, waitBytes);
    if (*(uint8 *)&resp > 1)
      return resp;
  }

  cmdTx[0] = cmd;
  cmdTx[1] = (arg & 0xFF000000) >> 24; // Little -> Big Endian the argument
  cmdTx[2] = (arg & 0x00FF0000) >> 16;
  cmdTx[3] = (arg & 0x0000FF00) >> 8;
  cmdTx[4] = (arg & 0x000000FF) >> 0;
  cmdTx[5] = (CRC_calcCRC7(CRC7_SDCARD_POLY, cmdTx, 5) << 1) | 0x01;

  sSDCard.cmdWaitClocks = SDCard_waitReady(0xFF, SD_MAX_WAIT_BUS_BYTES); // Wait for bus to be idle
  SPI_write(cmdTx, 6);
  do
  {
    SPI_read((uint8_t *)&resp, 1);  // Continuous read until the first byte is non-zero
  } while ((resp.filler) && (waitBytes--));

  return resp;
}

/**************************************************************************************************\
* FUNCTION    SDCard_initDisk
* DESCRIPTION Puts the SDCard into SPI mode, and prepares it to accept commands
* PARAMETERS  None
* RETURNS     TRUE if successful, FALSE otherwise
\**************************************************************************************************/
boolean SDCard_initDisk(void)
{
  uint8 OP_SD_SPI_MODE[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  SDCommandResponseR2 cardStatusResp;
  SDCommandResponseR7 ifCondResp;
  SDCommandResponseR1 cmdResp;
  SDCommandResponseR3 ocrResp;

  boolean success;
  uint32 trys = 0;

  SDCard_setup(TRUE); // Turn on the SPI and control pins
  SDCard_setState(SDCARD_STATE_SETUP); // Set the state and voltage

  // Send the SPI mode command with the chip deselected ... prime the pump if you will...
  SPI_write(OP_SD_SPI_MODE, sizeof(OP_SD_SPI_MODE));

  SELECT_CHIP_SD();
  do
  {
    success = FALSE; // Early breaks in the following sequence will result in a failure
    
    trys = 0;
    do
    {
      cmdResp = SDCard_sendCommand(GO_IDLE_STATE, 0x00000000, SD_MAX_WAIT_RESP_BYTES); // R1 reply
      if (cmdResp.idleState == 1)
        break;
    } while (trys++ < SD_MAX_WAIT_INIT_BYTES);
    if (trys >= SD_MAX_WAIT_INIT_BYTES)
      break;

    trys = 0;
    do
    {
      cmdResp = SDCard_sendCommand(SEND_IF_COND, 0x000001AA, SD_MAX_WAIT_RESP_BYTES);  // R1+R7 Reply
      SPI_read((uint8 *)&ifCondResp, sizeof(ifCondResp)); // Get that R7
      Util_swap32((uint32*)&ifCondResp); // Now flip it around...
      if ((cmdResp.idleState == 1) && (ifCondResp.echoBack == 0xAA) && (ifCondResp.voltageOk == 1))
        break;
      if ((cmdResp.illegalCmd == 1) && (cmdResp.filler == 0))
        break;  // CMD8 not supported -- move on!
    } while (trys++ < SD_MAX_WAIT_INIT_BYTES);
    if (trys >= SD_MAX_WAIT_INIT_BYTES)
      break;
    
    trys = 0;
    do
    {
      cmdResp = SDCard_sendCommand(SEND_OP_COND_SDC, 0x40000000, SD_MAX_WAIT_RESP_BYTES); // R1+R3 Reply
      if (cmdResp.idleState == 0)
        break;
    } while (trys++ < SD_MAX_WAIT_INIT_BYTES); // Continue until non-idle
    if (trys >= SD_MAX_WAIT_INIT_BYTES)
      break;

    trys = 0;
    do
    {
      cmdResp = SDCard_sendCommand(READ_OCR, 0x00000000, SD_MAX_WAIT_RESP_BYTES);
      SPI_read((uint8 *)&ocrResp, sizeof(ocrResp)); // Expect the next bytes to be ocrResp
      Util_swap32((uint32*)&ocrResp); // Now flip it around...
      if ((cmdResp.idleState == 0) && (ocrResp.powerUpStatus != 0))
      {
        sSDCard.isSDHC = ocrResp.capacityStatus;
        break;
      }
    } while (trys++ < SD_MAX_WAIT_INIT_BYTES);
    if (trys >= SD_MAX_WAIT_INIT_BYTES)
      break;

    trys = 0;
    do
    {
      cmdResp = SDCard_sendCommand(CARD_STATUS, 0x00000000, SD_MAX_WAIT_RESP_BYTES);  // R1+R2 Reply
      SPI_read((uint8 *)&cardStatusResp, sizeof(cardStatusResp));
      if ((cmdResp.idleState == 0) && (*(uint8 *)&cardStatusResp == 0))
        break;
    } while (trys++ < SD_MAX_WAIT_INIT_BYTES);
    if (trys >= SD_MAX_WAIT_INIT_BYTES)
      break;

    sSDCard.blockLen = 16;  // Set the block length to 128bits so we can read the CID and CSD registers
    if (SDCARD_RESPONSE_OK == SDCard_readBlock(SEND_CID, 0x00000000))
    {
      Util_copyMemory(&sSDCard.respBlock.arg.reference[0], (uint8 *)&sSDCard.sdCardId, sizeof(sSDCard.sdCardId));
      Util_swap32((uint32 *)&sSDCard.sdCardId.serial); // need to revend the serial #
    }
    if (SDCARD_RESPONSE_OK == SDCard_readBlock(SEND_CSD, 0x00000000))
    {
      Util_reverseBytes(&sSDCard.respBlock.arg.reference[4],  8);
      Util_reverseBytes(&sSDCard.respBlock.arg.reference[12], 4);
      Util_copyMemory(&sSDCard.respBlock.arg.reference[0], (uint8 *)&sSDCard.sdCardStatus, sizeof(sSDCard.sdCardStatus));
      sSDCard.cardBlockLen =     (1 << (sSDCard.sdCardStatus.readBlockLen));
      sSDCard.capacityMult = (1 << (sSDCard.sdCardStatus.deviceSizeMultiple + 2));
      sSDCard.cardCapacity = (sSDCard.capacityMult * sSDCard.cardBlockLen) *
                             (sSDCard.sdCardStatus.deviceSize + 1);
    }
    sSDCard.blockLen = 512; // Reset the block length to default

    success = TRUE; // If we have made it here then the sequence was successful
  } while(FALSE);
  DESELECT_CHIP_SD();

  SDCard_setup(FALSE); // Turn off the SPI and control pins
  SDCard_setState((success == TRUE) ? SDCARD_STATE_READY : SDCARD_STATE_IDLE); // Set state + volts
  return success;
}

/**************************************************************************************************\
* FUNCTION    SDCard_getDataBlock
* DESCRIPTION Upon receiving the specified token, reads a block into static memory
* PARAMETERS  token - response token to look for
* RETURNS     SDCARD_RESPONSE_OK if the read succeeds, various SDCommandResults otherwise
\**************************************************************************************************/
static SDCommandResult SDCard_getDataBlock(const uint8 token)
{
  SDDataErrorToken errorToken;
  uint16 cardCRC, calcCRC;
  uint8 cardToken;
  uint32 waitBytes = SD_MAX_WAIT_RESP_BYTES;

  do
  {
    SPI_read(&cardToken, 1); // Wait for the card to respond with either an error or the token
  } while ((cardToken != token) && (waitBytes-- > 1));
  if (waitBytes == 0)
    return SDCARD_TIMEOUT;  // return a timeout error if the error or token was not received

  if (cardToken != token)
  {
    errorToken = *(SDDataErrorToken *)&cardToken;
    if (errorToken.outOfRange)
      return SDCARD_OUT_OF_RANGE;
    else if (errorToken.cardECCFailed)
      return SDCARD_ECC_FAILED;
    else if (errorToken.cardControllerError)
      return SDCARD_CONTROLLER_ERROR;
    else if (errorToken.error)
      return SDCARD_DATA_ERROR;
    else
      return SDCARD_ERROR;
  }
  else
  {
    SPI_read((uint8 *)&sSDCard.respBlock, sSDCard.blockLen);
    SPI_read((uint8 *)&cardCRC, 2);
    Util_swap16(&cardCRC);  // Card is going to send the CRC BIG_ENDIAN
    calcCRC = CRC_crc16(0, CRC16_POLY_CCITT_STD, (uint8 *)&sSDCard.respBlock, sSDCard.blockLen);
    if (calcCRC != cardCRC)
      return SDCARD_BAD_CRC;
    else
      return SDCARD_RESPONSE_OK;
  }
}

/**************************************************************************************************\
* FUNCTION    SDCard_readBlock
* DESCRIPTION Reads a block out of the SDCard
* PARAMETERS  block - The SDCard block to read into local memory
* RETURNS     nothing
\**************************************************************************************************/
static SDCommandResult SDCard_readBlock(SDCommand cmd, uint32 arg)
{
  SDCommandResponseR1 cmdResp;
  SDCommandResult readResult;

  Util_swap32(&arg);
  arg &= 0xFFFFFE00;

  SELECT_CHIP_SD();
  sSDCard.preReadWaitClocks = SDCard_waitReady(0xFF, 65535);
  cmdResp = SDCard_sendCommand(cmd, arg, SD_MAX_WAIT_RESP_BYTES); // Wait for token in getDataBlock
  if (*(uint8 *)&cmdResp > 0)
    return SDCARD_COMMAND_ERROR;
  readResult = SDCard_getDataBlock(START_SINGLE_BLOCK_TOKEN);
  sSDCard.postReadWaitClocks = SDCard_waitReady(0xFF, 65535);
  DESELECT_CHIP_SD();
  return readResult;
}

/**************************************************************************************************\
* FUNCTION    SDCard_read
* DESCRIPTION Reads from the SDCard into the specified destination buffer
* PARAMETERS  pSrc  - pointer to source in SDCardMap
*             pDest - pointer to destination in SDCard
*             length - number of bytes to write
* RETURNS     true if the write succeeds
\**************************************************************************************************/
boolean SDCard_read(uint8 *pSrc, uint8 *pDest, uint16 length)
{
  uint32 block, offset;
  SDCommandResult readResult;

  if (length > 512)  // Currently can't handle multi-block reads
    return FALSE;
  if (sSDCard.state != SDCARD_STATE_READY) // Card must be initialized first
    return FALSE;

  block = (sSDCard.isSDHC) ? (uint32)pSrc >> 9 : (uint32)pSrc;
  offset = (uint32)pSrc & 0x0000001FF;

  SDCard_setState(SDCARD_STATE_READING); // Set the state and voltage
  SDCard_setup(TRUE); // Turn on the SPI and control pins
  readResult = SDCard_readBlock(READ_SINGLE_BLOCK, block);
  SDCard_setup(FALSE); // Turn off the SPI and control pins
  SDCard_setState(SDCARD_STATE_READY); // Set the state and voltage
  Util_copyMemory((uint8*)&sSDCard.respBlock + offset, pDest, length);

  return readResult == SDCARD_RESPONSE_OK;
}

/**************************************************************************************************\
* FUNCTION    SDCard_sendDataBlock
* DESCRIPTION Writes a buffer to the SDCard
* PARAMETERS  token - The token preceding the write
* RETURNS     SDCARD_RESPONSE_OK if the write succeeds, various SDCommandResults otherwise
\**************************************************************************************************/
static SDCommandResult SDCard_sendDataBlock(uint8 token)
{
  uint8  resp;
  uint16 crc = CRC_crc16(0, CRC16_POLY_CCITT_STD, (uint8 *)&sSDCard.respBlock, sSDCard.blockLen);
  Util_swap16(&crc);  // Card needs to receive the CRC BIG_ENDIAN

  SPI_write(&token, 1);
  SPI_write((uint8 *)&sSDCard.respBlock, sSDCard.blockLen);
  SPI_write((uint8 *)&crc, 2);

  SPI_read((uint8 *)&resp, 1);
  if ((resp & 0x1F) == 0x05)
    return SDCARD_RESPONSE_OK;
  else if ((resp & 0x0E) == 0x0A)
    return SDCARD_BAD_CRC;
  else if ((resp & 0x1F) == 0x0C)
    return SDCARD_WRITE_ERROR;
  else
    return SDCARD_ERROR;
}

/**************************************************************************************************\
* FUNCTION    SDCard_writeBlock
* DESCRIPTION Writes a block from local memory into the SDCard
* PARAMETERS  block - The block number to write
* RETURNS     SDCARD_RESPONSE_OK if the write succeeds, various SDCommandResults otherwise
\**************************************************************************************************/
static SDCommandResult SDCard_writeBlock(uint32 block, Delay *pDelay)
{
  SDCommandResponseR1 cardStatusRespR1, writeBlockRespR1;
  SDCommandResponseR2 cardStatusRespR2;
  SDCommandResult dataResp;

  Util_swap32(&block);
  block &= 0xFFFFFE00;

  SELECT_CHIP_SD();
  sSDCard.preWriteWaitClocks = SDCard_waitReady(0xFF, SD_MAX_WAIT_BUS_BYTES);
  writeBlockRespR1 = SDCard_sendCommand(WRITE_BLOCK, block, SD_MAX_WAIT_RESP_BYTES); // Result == 0
  dataResp = SDCard_sendDataBlock(START_SINGLE_BLOCK_TOKEN);
  sSDCard.preWriteWaitClocks = SDCard_waitReady(0xFF, SD_MAX_WAIT_PRE_WRITE_BYTES);
  
  SDCard_setState(SDCARD_STATE_WAITING); // Set the state and voltage
  SDCard_setup(FALSE); // Turn on the SPI and control pins
  Time_pendEnergyTime(pDelay); // Wait for the SDCard to complete the write
  SDCard_setup(TRUE); // Turn on the SPI and control pins
  SDCard_setState(SDCARD_STATE_WRITING); // Set the state and voltage
  SELECT_CHIP_SD();
  
  sSDCard.writeWaitClocks = SDCard_waitReady(0xFF, SD_MAX_WAIT_WRITE_BYTES); // Wait for bus release
  cardStatusRespR1 = SDCard_sendCommand(CARD_STATUS, 0x00000000, SD_MAX_WAIT_RESP_BYTES);
  SPI_read((uint8 *)&cardStatusRespR2, sizeof(cardStatusRespR2));
  sSDCard.postWriteWaitClocks = SDCard_waitReady(0xFF, SD_MAX_WAIT_BUS_BYTES);
  DESELECT_CHIP_SD();

  if (*(uint8 *)&writeBlockRespR1 > 0)
    return SDCARD_COMMAND_ERROR;
  else if ((*(uint8 *)&cardStatusRespR1 > 0) || (*(uint8 *)&cardStatusRespR2 > 0))
    return SDCARD_WRITE_ERROR;
  else
    return dataResp;
}

/**************************************************************************************************\
* FUNCTION    SDCard_write
* DESCRIPTION Writes a buffer to the the SDCard
* PARAMETERS   pSrc  - pointer to source RAM buffer
*              pDest - pointer to destination in SDCard
*             length - number of bytes to write
* RETURNS     TRUE if the write succeeds, FALSE otherwise
\**************************************************************************************************/
SDWriteResult SDCard_write(uint8 *pSrc, uint8 *pDest, uint16 length, Delay *pDelay)
{
  uint8 verify;
  uint32 block, offset;
  SDCommandResult readResult, writeResult, verifyResult;

  if (length > 512)  // Currently can't handle multi-block writes
    return SD_WRITE_RESULT_BAD_COMMAND;
  if (sSDCard.state != SDCARD_STATE_READY) // Card must be initialized first
    return SD_WRITE_RESULT_NOT_READY;

  // Read in the block that contains the data which will be overwritten
  block = (sSDCard.isSDHC) ? ((uint32)pDest >> 9) : ((uint32)pDest & 0xFFFFFE00);

  SDCard_setState(SDCARD_STATE_READING); // Set the state and voltage
  SDCard_setup(TRUE); // Turn on the SPI and control pins
  readResult = SDCard_readBlock(READ_SINGLE_BLOCK, block);

  // Copy the incoming data over top of whatever currently resides there
  offset = (uint32)pDest & 0x0000001FF;
  Util_copyMemory(pSrc, (uint8 *)&sSDCard.respBlock + offset, length);

  // Write the new contents of the block to the SDCard
  SDCard_setState(SDCARD_STATE_WRITING); // Set the state and voltage
  writeResult = SDCard_writeBlock(block, pDelay);

  // Verify that the source data now resides in the block
  SDCard_setState(SDCARD_STATE_VERIFYING); // Set the state and voltage
  verifyResult = SDCard_readBlock(READ_SINGLE_BLOCK, block);
  verify = Util_compareMemory(pSrc, (uint8 *)&sSDCard.respBlock + offset, length);

  SDCard_setup(FALSE); // Turn off the SPI and control pins
  SDCard_setState(SDCARD_STATE_READY); // Set the state and voltage

  if (readResult != SDCARD_RESPONSE_OK)
    return SD_WRITE_RESULT_READ_FAILED;
  else if (writeResult != SDCARD_RESPONSE_OK)
    return SD_WRITE_RESULT_WRITE_FAILED;
  else if ((verifyResult != SDCARD_RESPONSE_OK) || (verify != 0))
    return SD_WRITE_RESULT_VERIFY_FAILED;
  else
    return SD_WRITE_RESULT_OK;
}

/**************************************************************************************************\
* FUNCTION    SDCard_getStatusSDC
* DESCRIPTION txBuf: A pointer to the data to be transmitted. Can be NULL.
*             txLen: The number of bytes to be transmitted.
*             rxBuf: A pointer to the receive buffer. Can be NULL.
*             rxLen: The number of bytes to receive.
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
/*
static SDCommandResult SDCard_getStatusSDC(void)
{
  // Set Block length CMD(16) to 512bytes for reads (CMD17 single, CMD18 multi)
  SDCommandResponseR1 resp;
  SDCommandResult readResult;

  SELECT_CHIP_SD();
  resp = SDCard_sendCommand(SD_STATUS_SDC, 0x00000000, SD_MAX_WAIT_RESP_BYTES); // Result == 0
  readResult = SDCard_getDataBlock(sSDCard.respBlock.arg.reference, START_SINGLE_BLOCK_TOKEN);
  DESELECT_CHIP_SD();

  if (*(uint8 *)&resp > 0)
    return SDCARD_COMMAND_ERROR;
  else
    return readResult;
}
*/

/**************************************************************************************************\
* FUNCTION    SDCard_test
* DESCRIPTION Executes reads and writes to SDCard to test the code
* PARAMETERS  none
* RETURNS     nothing
\**************************************************************************************************/
bool SDCard_test(void)
{
  volatile uint16 crcNorm, crcRev;
  PowerCon_setDeviceDomain(DEVICE_EEPROM, VOLTAGE_DOMAIN_0);
  Time_delay(100000); // Wait 100ms for domains to settle
  SDCard_initDisk();
  return (sSDCard.cardCapacity > 0);
}
