#include "analog.h"
#include "i2c.h"
#include "powercon.h"
#include "si114x.h"
#include "time.h"
#include "types.h"
#include "util.h"
#include "stm32f4xx_hal.h"

// I2C slave address
#define SI114X_I2C_ADDRESS      (0x5A)

// I2C data buffer size
#define I2C_DATA_BUF_SIZE       25

// device command set
#define CMD_NOP                 0x00        // force a '0' into the response register
#define CMD_RESET               0x01        // force a reset of the device firmware
#define CMD_BUS_ADDR            0x02        // modify the I2C address
#define CMD_PS_FORCE            0x05        // force a single PS measurement
#define CMD_ALS_FORCE           0x06        //   "   "   "    ALS     "
#define CMD_PS_ALS_FORCE        0x07        //   "   "   "    PS/ALS  "
#define CMD_PS_PAUSE            0x09        // pause autonomous PS measurement
#define CMD_ALS_PAUSE           0x0A        //   "       "      ALS     "
#define CMD_PS_ALS_PAUSE        0x0B        //   "       "      PS/ALS  "
#define CMD_PS_AUTO             0x0D        // start     "      PS      "
#define CMD_ALS_AUTO            0x0E        //   "       "      ALS     "
#define CMD_PS_ALS_AUTO         0x0F        //   "       "      PS/ALS  "

// device "hardware key" value
#define DEV_HDW_KEY             0x17

// parameter operation flags
#define PARAM_RD                0x80
#define PARAM_SET               0xA0
#define PARAM_AND               0xC0
#define PARAM_OR                0xE0

// I2C registers
#define REG_PART_ID               0x00
#define REG_REV_ID                0x01
#define REG_SEQ_ID                0x02
#define REG_IRQ_CFG               0x03
#define REG_IRQ_ENABLE            0x04
#define REG_IRQ_MODE1             0x05
#define REG_IRQ_MODE2             0x06
#define REG_HW_KEY                0x07
#define REG_MEAS_RATE             0x08
#define REG_ALS_RATE              0x09
#define REG_PS_RATE               0x0A
#define REG_ALS_LO_TH             0x0B
#define REG_ALS_HI_TH             0x0D
#define REG_ALS_IR_ADCMUX         0x0E
#define REG_PS_LED21              0x0F
#define REG_PS_LED3               0x10
#define REG_PS1_TH                0x11
#define REG_PS2_TH                0x12
#define REG_PS3_TH                0x13
#define REG_PS_LED3_TH0           0x15
#define REG_PARAM_IN              0x17
#define REG_PARAM_WR              0x17
#define REG_COMMAND               0x18
#define REG_RESPONSE              0x20
#define REG_IRQ_STATUS            0x21
#define REG_ALS_VIS_DATA0         0x22
#define REG_ALS_VIS_DATA1         0x23
#define REG_ALS_IR_DATA0          0x24
#define REG_ALS_IR_DATA1          0x25
#define REG_PS1_DATA0             0x26
#define REG_PS1_DATA1             0x27
#define REG_PS2_DATA0             0x28
#define REG_PS2_DATA1             0x29
#define REG_PS3_DATA0             0x2A
#define REG_PS3_DATA1             0x2B
#define REG_AUX_DATA0             0x2C
#define REG_AUX_DATA1             0x2D
#define REG_PARAM_OUT             0x2E
#define REG_PARAM_RD              0x2E
#define REG_CHIP_STAT             0x30

// parameter offsets
#define PARAM_I2C_ADDR            0x00
#define PARAM_CH_LIST             0x01
#define PARAM_PSLED12_SELECT      0x02
#define PARAM_PSLED3_SELECT       0x03
#define PARAM_FILTER_EN           0x04
#define PARAM_PS_ENCODING         0x05
#define PARAM_ALS_ENCODING        0x06
#define PARAM_PS1_ADC_MUX         0x07
#define PARAM_PS2_ADC_MUX         0x08
#define PARAM_PS3_ADC_MUX         0x09
#define PARAM_PS_ADC_COUNTER      0x0A
#define PARAM_PS_ADC_CLKDIV       0x0B
#define PARAM_PS_ADC_GAIN         0x0B
#define PARAM_PS_ADC_MISC         0x0C
#define PARAM_RESERVED            0x0D
#define PARAM_ALS_IR_ADC_MUX      0x0E
#define PARAM_AUX_ADC_MUX         0x0F
#define PARAM_ALSVIS_ADC_COUNTER  0x10
#define PARAM_ALSVIS_ADC_CLKDIV   0x11
#define PARAM_ALSVIS_ADC_GAIN     0x11
#define PARAM_ALSVIS_ADC_MISC     0x12
#define PARAM_ALS_HYST            0x16
#define PARAM_PS_HYST             0x17
#define PARAM_PS_HISTORY          0x18
#define PARAM_ALS_HISTORY         0x19
#define PARAM_ADC_OFFSET          0x1A
#define PARAM_SLEEP_CTRL          0x1B
#define PARAM_LED_RECOVERY        0x1C
#define PARAM_ALSIR_ADC_COUNTER   0x1D
#define PARAM_ALSIR_ADC_CLKDIV    0x1E
#define PARAM_ALSIR_ADC_GAIN      0x1E
#define PARAM_ALSIR_ADC_MISC      0x1F

// LED current values
#define LEDI_000                  0x00
#define LEDI_006                  0x01
#define LEDI_011                  0x02
#define LEDI_022                  0x03
#define LEDI_045                  0x04
#define LEDI_067                  0x05
#define LEDI_090                  0x06
#define LEDI_112                  0x07
#define LEDI_135                  0x08
#define LEDI_157                  0x09
#define LEDI_180                  0x0A
#define LEDI_202                  0x0B
#define LEDI_224                  0x0C
#define LEDI_269                  0x0D
#define LEDI_314                  0x0E
#define LEDI_359                  0x0F
#define MIN_LED_CURRENT           LEDI_000
#define MAX_LED_CURRENT           LEDI_359

// PARAM_CH_LIST
#define PS1_TASK                  0x01
#define PS2_TASK                  0x02
#define PS3_TASK                  0x04
#define ALS_VIS_TASK              0x10
#define ALS_IR_TASK               0x20
#define AUX_TASK                  0x40

#define NO_LED                    0x00
#define LED1_EN                   0x01
#define LED2_EN                   0x02
#define LED3_EN                   0x04
#define SEL_LED1_PS1              (LED1_EN)
#define SEL_LED2_PS1              (LED2_EN)
#define SEL_LED3_PS1              (LED3_EN)
#define SEL_LED1_PS2              (LED1_EN<<4)
#define SEL_LED2_PS2              (LED2_EN<<4)
#define SEL_LED3_PS2              (LED3_EN<<4)
#define SEL_LED1_PS3              (LED1_EN)
#define SEL_LED2_PS3              (LED2_EN)
#define SEL_LED3_PS3              (LED3_EN)

#define PS1_LSB                   0x10
#define PS2_LSB                   0x20
#define PS3_LSB                   0x40

#define ALS_IR_LSB                0x20

#define HSIG_EN                   0x20

// ADC Mux Settings
#define MUX_SMALL_IR              0x00
#define MUX_LARGE_IR              0x03

// Hardware Key value (placed in REG_HW_KEY)
#define HW_KEY_VAL0               0x17

#define SI114X_HIGH_SPEED_VMIN (2.7)
#define SI114X_LOW_SPEED_VMIN (2.0)

typedef enum
{
  LED_MODE_NONE           = 0,
  LED_MODE_SINGLE_LED     = 1,
  LED_MODE_HI_LO_SEQUENCE = 2,
  LED_MODE_ONE_SHOT       = 3,
  LED_MODE_SEQUENTIAL_LED = 4,
  LED_MODE_LO_HI_SEQUENCE = 5
} LEDMode;

#define HIH_HIGH_SPEED_VMIN (2.7)
#define HIH_LOW_SPEED_VMIN (2.0)

// LED drive current settings: 0-15 = 0-359mA
static const uint8_t SI114X_LED_DRIVE[16] = 
{
 LEDI_000, LEDI_006, LEDI_011, LEDI_022, LEDI_045, LEDI_067, LEDI_090, LEDI_112, 
 LEDI_135, LEDI_157, LEDI_180, LEDI_202, LEDI_224, LEDI_269, LEDI_314, LEDI_359
};

// Power profile voltage definitions, in HIHPowerProfile / HIHState order
static const double SI114X_POWER_PROFILES[SI114X_PROFILE_MAX][SI114X_STATE_MAX] =
{ // Idle, Data Ready, Transmitting, Waiting, Reading
  {3.3, 3.3, 3.3, 3.3, 3.3},  // Standard profile
  {2.9, 2.9, 2.9, 2.9, 3.3},  // 29VIRyTW
  {2.5, 2.5, 2.5, 2.5, 3.3},  // 25VIRyTW
  {2.3, 2.3, 2.3, 2.3, 3.3},  // 23VIRyTW
};

static struct
{ 
  boolean     isInitialized;
  double      vDomain[SI114X_STATE_MAX]; // The domain voltage for each state
  Si114xState    state;
  
  bool   ps1SmallPD; // true = Use SmallPD, false = Use LargePD, 
  bool   ps2SmallPD;
  bool   ps3SmallPD;
  bool   alsSmallPD;
  
  bool   psxHiSigEn;  // true = High signal enable
  bool   alsHiSigEn;
  
  bool   alsUpper16;  // true = Return upper16bits of conversion result
  bool   ps1Upper16;
  bool   ps2Upper16;
  bool   ps3Upper16;
  
  uint8_t  hwKey;
  uint8_t  partID;  // part/sequencer IDs & serial number
  uint8_t  seqID;
  uint32_t serNum;
  
  uint8_t  psxGain;   // gains/drive levels
  uint8_t  alsGain;
  uint8_t  driveLed1;
  uint8_t  driveLed2;
  uint8_t  driveLed3;
  LEDMode  ledMode;   // LED flashing sequence
  
  uint8_t  regPSLED21; // PSLED21 "shadow" register
  
  uint16_t lightPSIR1; // storage for the most recent sensor readings
  uint16_t lightPSIR2;
  uint16_t lightPSIR3;
  uint16_t lightALSIR;
} sSi114x;

static bool Si114x_ModeSpecificInit(void);
static bool ParamUpdate(uint8_t addr, uint8_t val, uint8_t logOp);
static bool Si114x_SetLEDDrive(uint8_t led, uint8_t level);
static bool Si114x_SwitchGain(uint8_t alsGain, uint8_t psxGain);
static bool Si114xReset(void);
boolean Si114x_setup(boolean state);

/*************************************************************************************************\
*  FUNCTION:    Si114x_i2cRead
*  DESCRIPTION: Writes register data to the Si114x
*  PARAMETERS:  regAddr: The address of the register to be written
*               
*  RETURNS:     true if successful, (ERR) otherwise
\*************************************************************************************************/
bool Si114x_i2cRead(uint8_t regAddr, void *pDest, uint32_t len)
{
  Si114x_setup(true);
  I2C_memRead(SI114X_I2C_ADDRESS, regAddr, pDest, len);
  Si114x_setup(false);
  return true;
}

/*************************************************************************************************\
*  FUNCTION:    Si114x_i2cWrite
*  DESCRIPTION: Writes register data to the Si114x
*  PARAMETERS:  regAddr: The address of the register to be written
*               regVal:  The value to write to the register
*               delay:   An optional delay to apply after the write completes
*  RETURNS:     true if successful, (ERR) otherwise
\*************************************************************************************************/
bool Si114x_i2cWrite(uint8_t regAddr, uint8_t regVal, uint32_t delay)
{
  Si114x_setup(true);
  I2C_memWrite(SI114X_I2C_ADDRESS, regAddr, &regVal, 1);
  Time_delay(delay * 1000);
  Si114x_setup(false);
  return true;
}

/*************************************************************************************************\
*  FUNCTION:    Si114x_writeCmd
*  DESCRIPTION: Writes data to the command register of the device. it must be done in a specific
*               manner and therefore warrants its own function. This is documented in the datasheet
*               under the command section and references the PARAMETER_RAM section
*  PARAMETERS:  regVal:  The value to write to the register
*               delay:   An optional delay to apply after the write completes
*  RETURNS:     true if successful, (ERR) otherwise
\*************************************************************************************************/
bool Si114x_writeCmd(uint8_t value, uint32_t delay)
{
  uint8_t result, retries = 0xFF;
  do
  {
    Si114x_i2cWrite(REG_COMMAND, CMD_NOP, 0);
    Si114x_i2cRead(REG_COMMAND, &result, 1);
  } while ((0x00 != result) && (--retries > 0));
  if (0 == retries)
    return false;
  else
    retries = 0xFF;
  
  if (CMD_NOP == value)
    return true;
  
  Si114x_i2cWrite(REG_COMMAND, value, delay);
  if (CMD_RESET == value)  // Datasheet indicates that the RESP reg will not increment on a reset
    return true;
  
  // Read the response register until it returns 0 indicating that the command was processed
  do {Si114x_i2cRead(REG_COMMAND, &result, 1); } while ((0x00 == result) && (--retries > 0));
  if (retries > 0) 
    return true;
  else
    return false;
}

/**************************************************************************************************\
* FUNCTION    SI114X_init
* DESCRIPTION Initializes the HIH613X module
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SI114X_init(void)
{  
  memset(&sSi114x, 0x00, sizeof(sSi114x));
  // set the defaults for the PS1-3 photo diode & data encoding options
  sSi114x.alsSmallPD = false;
  sSi114x.ps1SmallPD = false;
  sSi114x.ps2SmallPD = false;
  sSi114x.ps3SmallPD = false;
  
  // Enable the Hi-Signal mode for earbud builds
  sSi114x.alsHiSigEn = true;
  sSi114x.psxHiSigEn = true;
  
  sSi114x.alsUpper16 = true;
  sSi114x.ps1Upper16 = true;
  sSi114x.ps2Upper16 = true;
  sSi114x.ps3Upper16 = true;
  
  // set the default sensor gains
  sSi114x.psxGain = 1;
  sSi114x.alsGain = 1;
  
  // set the default LED drive levels
  sSi114x.driveLed1  = LEDI_045;
  sSi114x.driveLed2  = LEDI_045;
  sSi114x.driveLed3  = LEDI_045;
  
  sSi114x.ledMode = LED_MODE_SINGLE_LED;

  // reset the remaining variables
  sSi114x.regPSLED21 = ((SI114X_LED_DRIVE[sSi114x.driveLed2] << 4) | 
                        (SI114X_LED_DRIVE[sSi114x.driveLed1] << 0));
  sSi114x.partID     = 0;
  sSi114x.seqID      = 0;
  sSi114x.lightPSIR1 = 0;
  sSi114x.lightPSIR2 = 0;
  sSi114x.lightPSIR3 = 0;
  sSi114x.lightALSIR = 0;
  sSi114x.serNum     = 0;
  
  Si114xReset();
  Si114x_ModeSpecificInit(); // complete the hardware initialization
}

/**************************************************************************************************\
* FUNCTION    Si114x_setup
* DESCRIPTION Enables or Disables the peripheral pins required to operate the HIH613X sensor
* PARAMETERS  state: If TRUE, required peripherals will be enabled. Otherwise control pins will be
*                    set to input.
* RETURNS     TRUE
* NOTES       Also configures the state of the I2C pins
\**************************************************************************************************/
boolean Si114x_setup(boolean state)
{
  if (sSi114x.vDomain[state] > SI114X_HIGH_SPEED_VMIN)
    I2C_setup(state, I2C_CLOCK_RATE_100000);  // Configure the I2C lines for HS txrx
  else if (sSi114x.vDomain[state] > SI114X_LOW_SPEED_VMIN)
    I2C_setup(state, I2C_CLOCK_RATE_050000);  // Configure the I2C lines for LS txrx
  else
    I2C_setup(state, I2C_CLOCK_RATE_025000);  // Configure the I2C lines for XLS txrx
  return TRUE;
}

/**************************************************************************************************\
* FUNCTION    SI114X_ReadAndMeasure
* DESCRIPTION Interfaces with the SI114X proximity sensor
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
void SI114X_ReadAndMeasure(void *pData)
{
  uint16_t lightRead[4];
  Si114x_i2cRead(REG_ALS_IR_DATA0, lightRead, 8); // Read (ALS-IR + PS1-IR + PS2-IR + PS3-IR)
  sSi114x.lightALSIR = lightRead[0];
  sSi114x.lightPSIR1 = lightRead[1];
  sSi114x.lightPSIR2 = lightRead[2];
  sSi114x.lightPSIR3 = lightRead[3];
  Si114x_writeCmd(CMD_PS_ALS_FORCE, 0);
}

//====================================================================
//  FUNCTION:       ParamUpdate
//
//  DESCRIPTION:    Update a specified device parameter using the
//                  supplied value.
//
//  PARAMETERS:     addr  - parameter address
//                  val   -     "     value
//                  logOp - logical operation (Set, OR, AND)
//
//  RETURNS:        Pass: true
//                  Fail: (-ERR)
//====================================================================
static bool ParamUpdate(uint8_t addr, uint8_t val, uint8_t logOp)
{
  uint8_t cmd;
  uint8_t result;
  
  // Send the Parameter Value:
  if (!Si114x_i2cWrite(REG_PARAM_WR, val, 0))
    return false;
  
  // Send the Operation Command: (command based on the operation requested)
  cmd = (logOp + (addr & 0x1F));
  if (!Si114x_writeCmd(cmd, 0))
    return false;
  
  // Send the Read Operation Command: (command based on the operation requested)
  cmd = (PARAM_RD + (addr & 0x1F));
  if (!Si114x_writeCmd(cmd, 0))
    return false;
  
  // Read & Verify the Parameter Value:
  if (!Si114x_i2cRead(REG_PARAM_RD, &result, 1))
    return false;
  
  switch (logOp)
  {
    case PARAM_AND:
      if ((result & (~val)) != 0)
      {
          return false;
      }
      break;
    case PARAM_OR:
      if ((result & val) != val)
      {
          return false;
      }
      break;
    default:  // Must be a PARAM_SET
      if (result != val)
      {
          return false;
      }
      break;
  }
  return true;
}
  
/*************************************************************************************************\
*  FUNCTION:    Si114x_SetLEDDrive
*  DESCRIPTION: Changes the LED drive levels
*  PARAMETERS:  led   - the LED drive level to change
*               level - the level to which the led drive will be changed
*  RETURNS:     true if successful, (ERR) otherwise
\*************************************************************************************************/
static bool Si114x_SetLEDDrive(uint8_t led, uint8_t level)
{
  level &= 0x0F; // limit the setting level
  switch (led)
  {
    case 1:
      sSi114x.driveLed1  = level;
      sSi114x.regPSLED21 = ((sSi114x.regPSLED21 & 0xF0) | ((SI114X_LED_DRIVE[level] << 0) & 0x0F)); 
      return Si114x_i2cWrite(REG_PS_LED21, sSi114x.regPSLED21, 0);
    case 2: 
      sSi114x.driveLed2  = level;
      sSi114x.regPSLED21 = ((sSi114x.regPSLED21 & 0x0F) | ((SI114X_LED_DRIVE[level] << 4) & 0xF0));
      return Si114x_i2cWrite(REG_PS_LED21, sSi114x.regPSLED21, 0);
    case 3:
      sSi114x.driveLed3  = level;
      return Si114x_i2cWrite(REG_PS_LED3, SI114X_LED_DRIVE[level], 0);
    default: 
      return false;
  }
}

/*************************************************************************************************\
*  FUNCTION:    Si114x_SetLEDMode
*  DESCRIPTION: Changes the LED / PhotoSensor sequences to match the input parameter
*  PARAMETERS:  mode - The resulting sequence
*  RETURNS:     true if successful, (ERR) otherwise
\*************************************************************************************************/
static int16_t Si114x_SetLEDMode(LEDMode mode)
{
  uint8_t ps12Val, ps3Val, taskVal;    // Proximity sensors 1 and 2 are combined, 3 is separate
  int16_t status12, status3, statusT;  
  
  switch (mode)
  {
    case LED_MODE_HI_LO_SEQUENCE: // Multiple LED debug configuration mode 123,12,1
      taskVal = (PS1_TASK | PS2_TASK | PS3_TASK | ALS_IR_TASK);
      ps12Val = (SEL_LED1_PS1 | SEL_LED2_PS1 | SEL_LED3_PS1 | SEL_LED1_PS2 | SEL_LED2_PS2);
      ps3Val  = (SEL_LED1_PS3);
      break;
    case LED_MODE_SEQUENTIAL_LED: // Each LED corresponds to each PS (Sequential)
      taskVal = (PS1_TASK | PS2_TASK | PS3_TASK | ALS_IR_TASK);
      ps12Val = (SEL_LED1_PS1 | SEL_LED2_PS2);
      ps3Val  = (SEL_LED3_PS3);
      break;
    case LED_MODE_ONE_SHOT: // Multiple LEDs, one shot of all LEDs at PS1 slot
      taskVal = (PS1_TASK | ALS_IR_TASK);
      ps12Val = (SEL_LED1_PS1 | SEL_LED2_PS1 | SEL_LED3_PS1);
      ps3Val  = (NO_LED);
      break;
    case LED_MODE_LO_HI_SEQUENCE: // Multiple LEDs, old debug configuration, 1,12,123
      taskVal = (PS1_TASK | PS2_TASK | PS3_TASK | ALS_IR_TASK);
      ps12Val = (SEL_LED1_PS1 | SEL_LED1_PS2 | SEL_LED2_PS2);
      ps3Val  = (SEL_LED1_PS3 | SEL_LED2_PS3 | SEL_LED3_PS3);
      break;
    case LED_MODE_SINGLE_LED: // Single LED operating mode
      taskVal = (PS1_TASK | ALS_IR_TASK);
      ps12Val = (SEL_LED1_PS1);
      ps3Val  = (NO_LED);
      break;
    default: // Invalid LED mode selection
      return false;  // No update occurs if passed an invalid parameter
  }
  
  // Attempt to write parameters (don't bail out early if one fails)
  status12 = ParamUpdate(PARAM_PSLED12_SELECT, ps12Val, PARAM_SET);
  status3  = ParamUpdate(PARAM_PSLED3_SELECT,  ps3Val,  PARAM_SET);
  statusT  = ParamUpdate(PARAM_CH_LIST,        taskVal, PARAM_SET);
  if ((status12 != true) || (status3 != true) || (statusT != true))
    return (false);
  
  // If the change was successful then store the new LED mode
  sSi114x.ledMode = mode;
  return true;
}

/*************************************************************************************************\
*  FUNCTION:    Si114x_SwitchGain
*  DESCRIPTION: Changes the gain of the detector to the specified value
*  PARAMETERS:  psxGain: The value to which the photosensor gain will be changed
*               alGain: The value to which the ambient light gain will be changed
*  RETURNS:     N/A
\*************************************************************************************************/
static bool Si114x_SwitchGain(uint8_t alsGain, uint8_t psxGain)
{
  return (ParamUpdate(PARAM_ALSIR_ADC_GAIN, alsGain, PARAM_SET) &&
          ParamUpdate(PARAM_PS_ADC_GAIN,    psxGain, PARAM_SET));
}

/*************************************************************************************************\
*  FUNCTION:    Si114x_ModeSpecificInit
*  DESCRIPTION: Sets the parameters on the Si114x according to the current configuration
*  PARAMETERS:  None (Configuration is pulled from the module static structure)
*  RETURNS:     true if successful (ERR) otherwise
\*************************************************************************************************/
static bool Si114x_ModeSpecificInit(void)
{
  int16_t  status;
  
  if (sSi114x.ps1SmallPD) // Enable the small photo-diode for the PS1 channel
    status = ParamUpdate(PARAM_PS1_ADC_MUX, MUX_SMALL_IR, PARAM_SET);
  else
    status = ParamUpdate(PARAM_PS1_ADC_MUX, MUX_LARGE_IR, PARAM_SET);
  if (status != true)
    return false;
  
  if (sSi114x.ps2SmallPD) // Enable the small photo-diode for the PS2 channel
    status = ParamUpdate(PARAM_PS2_ADC_MUX, MUX_SMALL_IR, PARAM_SET);
  else
    status = ParamUpdate(PARAM_PS2_ADC_MUX, MUX_LARGE_IR, PARAM_SET);
  if (status != true)
    return false;
  
  if (sSi114x.ps3SmallPD) // Enable the small photo-diode for the PS3 channel
    status = ParamUpdate(PARAM_PS3_ADC_MUX, MUX_SMALL_IR, PARAM_SET);
  else
    status = ParamUpdate(PARAM_PS3_ADC_MUX, MUX_LARGE_IR, PARAM_SET);
  if (status != true)
    return false;
  
  if (sSi114x.alsSmallPD) // Enable the small photo-diode for the Ambient Light channel
    status = ParamUpdate(PARAM_ALS_IR_ADC_MUX, MUX_SMALL_IR, PARAM_SET);
  else
    status = ParamUpdate(PARAM_ALS_IR_ADC_MUX, MUX_LARGE_IR, PARAM_SET);
  if (status != true)
    return false;
  
  if (sSi114x.psxHiSigEn) // Apply the high-signal enable parameter to PS readings
    status = ParamUpdate(PARAM_PS_ADC_MISC, HSIG_EN, PARAM_OR);
  else
    status = ParamUpdate(PARAM_PS_ADC_MISC, (~HSIG_EN), PARAM_AND);
  if (status != true)
    return false;
  
  if (sSi114x.alsHiSigEn) // Apply the high-signal enable parameter to ALS readings
    status = ParamUpdate(PARAM_ALSIR_ADC_MISC, HSIG_EN, PARAM_OR);
  else
    status = ParamUpdate(PARAM_ALSIR_ADC_MISC, (~HSIG_EN), PARAM_AND);
  if (status != true)
    return false;
  
  // Ensure LSB reporting is turned off if we're using the upper 16 bits (defaults to MSBs)
  if (sSi114x.ps1Upper16)
    status = ParamUpdate(PARAM_PS_ENCODING, (~PS1_LSB), PARAM_AND);
  else  // Otherwise turn on the LSB reporting
    status = ParamUpdate(PARAM_PS_ENCODING, PS1_LSB, PARAM_OR);
  if (status != true)
    return false;
  
  // Ensure LSB reporting is turned off if we're using the upper 16 bits (defaults to MSBs)
  if (sSi114x.ps2Upper16)
    status = ParamUpdate(PARAM_PS_ENCODING, (~PS2_LSB), PARAM_AND);
  else // Otherwise turn on the LSB reporting
    status = ParamUpdate(PARAM_PS_ENCODING, PS2_LSB, PARAM_OR);
  if (status != true)
    return false;
  
  // Ensure LSB reporting is turned off if we're using the upper 16 bits (defaults to MSBs)
  if (sSi114x.ps3Upper16)
    status = ParamUpdate(PARAM_PS_ENCODING, (~PS3_LSB), PARAM_AND);
  else // Otherwise turn on the LSB reporting
    status = ParamUpdate(PARAM_PS_ENCODING, PS3_LSB, PARAM_OR);
  if (status != true)
    return false;
  
  // Ensure LSB reporting is turned off if we're using the upper 16 bits (defaults to MSBs)
  if (sSi114x.alsUpper16)
    status = ParamUpdate(PARAM_ALS_ENCODING, (~ALS_IR_LSB), PARAM_AND);
  else // Otherwise turn on the LSB reporting
    status = ParamUpdate(PARAM_ALS_ENCODING, ALS_IR_LSB, PARAM_OR);
  if (status != true)
    return false;
  
  // ...set the LED drives
  Si114x_SetLEDDrive(1, sSi114x.driveLed1);
  Si114x_SetLEDDrive(2, sSi114x.driveLed2);
  Si114x_SetLEDDrive(3, sSi114x.driveLed3);
  Si114x_SwitchGain(sSi114x.alsGain, sSi114x.psxGain); // set the sensor gain
  Si114x_SetLEDMode(sSi114x.ledMode); // Configure the LED mode (LED to Proximity Sensor sequence mapping)
  
  // Force the First Conversion Cycle:
  if (true != Si114x_writeCmd(CMD_PS_ALS_FORCE, 0))
    return false;
  
  return true;
}

//====================================================================
//  FUNCTION:       Si114xReset
//
//  DESCRIPTION:    
//
//  PARAMETERS:     None
//
//  RETURNS:        status
//====================================================================
static bool Si114xReset(void)
{
  if (true != Si114x_writeCmd(CMD_RESET, 1))  // Send a RESET
    return false;
  
  if (true != Si114x_i2cWrite(REG_HW_KEY, DEV_HDW_KEY, 0))
    return false;
  
  if (true != Si114x_i2cWrite(REG_MEAS_RATE, 0x00, 0))
    return false;
     
  if (true != Si114x_i2cWrite(REG_IRQ_ENABLE, 0x00, 0))
    return false;
      
  if (true != Si114x_i2cWrite(REG_IRQ_MODE1, 0x00, 0))
    return false;
      
  if (true != Si114x_i2cWrite(REG_IRQ_MODE2, 0x00, 0))
    return false;
      
  if (true != Si114x_i2cWrite(REG_IRQ_CFG, 0x00, 0))
    return false;
  
  if (true != Si114x_i2cWrite(REG_IRQ_STATUS, 0x00, 0))
    return false;
    
  return true;
}

/**************************************************************************************************\
* FUNCTION    SI114X_test
* DESCRIPTION Interfaces with the SI114X proximity sensor
* PARAMETERS  None
* RETURNS     Nothing
\**************************************************************************************************/
bool SI114X_test(void)
{
  if (!Si114x_i2cRead(REG_PART_ID, &sSi114x.partID, sizeof(sSi114x.partID))) // Part ID
    return false;
  if (!Si114x_i2cRead(REG_SEQ_ID, &sSi114x.seqID,  sizeof(sSi114x.seqID)))  // Sequencer ID
    return false;
  
  switch (sSi114x.partID)
  {
    case 0x41:
    case 0x42:
    case 0x43:
        break;
    default:
      return false;
  }  
  return true;
}
