#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "uart.h"
#include "util.h"

#define FILE_ID ANALOG_C

// Static definitions
#define ANALOG_TPS62240_VREF ((double)0.600000000)
#define ANALOG_LM2623_VREF ((double)1.240000000)

#define ANALOG_MAX4378_GAIN ((double)100.0)

// Macro Definitions
#define SELECT_DOMLEN()   do { GPIOE->BSRRH |= 0x00000004; } while (0)
#define DESELECT_DOMLEN() do { GPIOE->BSRRL |= 0x00000004; } while (0)

const DomainConfig DEFAULT_CONFIGURATION[NUM_ANALOG_DOMAINS] = {
//Regulator,   inAmp,  outAmp,     r1,     r2,     rf,  vMax, vMin,  rIn, rOut
  {TPS62240, MAX4378, MAX4378, 360000, 180000, 144000,   3.3,  1.8, 10.0, 10.0}, // MCU_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // ANALOG_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // SRAM_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // SPI_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // ENERGY_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // COMMS_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // IO_DOMAIN
  {TPS62240, MAX4378, MAX4378, 360000,  75000, 144000,   3.6,  0.6, 10.0, 10.0}, // BUCK_DOMAIN7
  {LM2623,   MAX4378, MAX4378, 125000,  75000,  60000,   5.7,  2.0, 10.0, 10.0}, // BOOST_DOMAIN0
  {LM2623,   MAX4378, MAX4378, 125000,  75000,  60000,   5.7,  2.0, 10.0, 10.0}, // BOOST_DOMAIN1
  {LM2623,   MAX4378, MAX4378, 125000,  75000,  60000,   5.7,  2.0, 10.0, 10.0}, // BOOST_DOMAIN2
  {LM2623,   MAX4378, MAX4378, 125000,  75000,  60000,   5.7,  2.0, 10.0, 10.0}, // BOOST_DOMAIN3
  {LM2623,   MAX4378, MAX4378, 125000,  75000,  60000,   5.7,  2.0, 10.0, 10.0}, // BOOST_DOMAIN4
  {LM2623,   MAX4378, MAX4378, 180000,  75000,  22000,  12.0,  0.6, 10.0, 10.0}, // BOOST_DOMAIN5
  {LM2623,   MAX4378, MAX4378, 180000,  75000,  22000,  12.0,  0.6, 10.0, 10.0}, // BOOST_DOMAIN6
  {LM2623,   MAX4378, MAX4378, 180000,  75000,  22000,  12.0,  0.6, 10.0, 10.0}, // BOOST_DOMAIN7
};

// Static structures
struct
{
  DomainStatus domainStatus[NUM_ANALOG_DOMAINS];
  DomainConfig domainConfig[NUM_ANALOG_DOMAINS];
} sAnalog;

// Local function prototypes
static double Analog_getReferenceVoltage(RegulatorType type);
static double Analog_getFeedbackVoltage(VoltageDomain domain, double vDomain);
static double Analog_getOutputVoltage(VoltageDomain domain, double vFeedback);

/*************************************************************************************************\
* FUNCTION    Analog_getReferenceVoltage
* DESCRIPTION Returns the reference voltage for a particular regulator type
* PARAMETERS  type: The type of regulator to investigate
* RETURNS     The reference voltage of the selected regulator type
* NOTES       None
\*************************************************************************************************/
static double Analog_getReferenceVoltage(RegulatorType type)
{
  switch (type)
  {
    case TPS62240:
      return ANALOG_TPS62240_VREF;
    case LM2623:
      return ANALOG_LM2623_VREF;
    default:
      return 0.0;
  }
}

/*************************************************************************************************\
* FUNCTION    Analog_init
* DESCRIPTION Initializes the pins and data structures required for analog measurement and control
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_init(void)
{
  uint32 i;
  const uint16 ctrlPortB = GPIO_Pin_0;
  const uint16 ctrlPortC = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
  const uint16 ctrlPortE = GPIO_Pin_2;

  GPIO_InitTypeDef analogCtrlPortB = {ctrlPortB, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef analogCtrlPortC = {ctrlPortC, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef analogCtrlPortE = {ctrlPortE, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &analogCtrlPortB);

  GPIO_setPortClock(GPIOC, TRUE);
  GPIO_configurePins(GPIOC, &analogCtrlPortC);

  GPIO_setPortClock(GPIOE, TRUE);
  GPIO_configurePins(GPIOE, &analogCtrlPortE);

  Util_fillMemory(&sAnalog, sizeof(sAnalog), 0x00);
  // Set up each domain
  for (i = 0; i < NUM_ANALOG_DOMAINS; i++)
    Analog_setup((VoltageDomain)i, DEFAULT_CONFIGURATION[i]);
}

/*************************************************************************************************\
* FUNCTION    Analog_setup
* DESCRIPTION Configures the specified domain according to the supplied configuration
* PARAMETERS  domain: A voltage domain to configure
*             config: The configuration for the selected voltage domain
* RETURNS     TRUE if setup was successful, FALSE otherwise
* NOTES       The function will not enable the domain
\*************************************************************************************************/
void Analog_setup(VoltageDomain domain, DomainConfig config)
{
  sAnalog.domainConfig[domain] = config;

  sAnalog.domainStatus[domain].isEnabled       = FALSE;
  sAnalog.domainStatus[domain].feedbackVoltage = Analog_getReferenceVoltage(config.regulator);
  sAnalog.domainStatus[domain].domainVoltage   = 0;
  sAnalog.domainStatus[domain].inputCurrent    = 0;
  sAnalog.domainStatus[domain].outputCurrent   = 0;
}

/*************************************************************************************************\
* FUNCTION    Analog_analogSelect
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  domain: The domain to enable or disable
              domen:  Selects the inhibit of DACOUT1 for voltage adjustments
* RETURNS     Nothing
* NOTES       DOMRST and DOMLEN are unaffected
\*************************************************************************************************/
void Analog_selectChannel(VoltageDomain chan, boolean domen)
{ 
  union
  {
    struct
    {
      uint32 domen   : 1;
      uint32 domsel1 : 1;
      uint32 domsel2 : 1;
      uint32 domsel0 : 1;
      uint32 domsel3 : 1;
      uint32 domrst  : 1;
      uint32 filler  : 26;
    } asBfldLH;
    uint32 asWord;
    uint8  asBytes[4];
  } portCLSB;

  portCLSB.asWord = 0;
  portCLSB.asBfldLH.domen   = (domen == FALSE)          ? 0 : 1;
  portCLSB.asBfldLH.domsel0 = ((chan & 0x00000001) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel1 = ((chan & 0x00000002) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel2 = ((chan & 0x00000004) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel3 = ((chan & 0x00000008) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domrst  = (1);  // keep domrst high so we dont latch->demux the 259s
  GPIOC->ODR = (GPIOC->ODR & 0xFFC0) | (portCLSB.asWord & 0x003F);
}

/*************************************************************************************************\
* FUNCTION    Analog_setDomain
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  domain: The domain to enable or disable
              state: The state in which to put the selected domain
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
boolean Analog_setDomain(VoltageDomain domain, boolean state, double vOut)
{
  double fbVoltage = Analog_getFeedbackVoltage(domain, vOut);

  // If a task is requesting a dangerous range then NAK the request
  if ((vOut > sAnalog.domainConfig[domain].vMax) || (vOut < sAnalog.domainConfig[domain].vMin))
    return FALSE;

  switch (domain)
  {
    case MCU_DOMAIN:
      break; // Can't enable or disable the MCU domain
    case ANALOG_DOMAIN:  // V1EN is connected to a pin PB0
      if (state)
        GPIOB->BSRRL = 0x0001;
      else
        GPIOB->BSRRH = 0x0001;
      break;
    case SRAM_DOMAIN:
    case SPI_DOMAIN:
    case ENERGY_DOMAIN:
    case COMMS_DOMAIN:
    case IO_DOMAIN:
    case BUCK_DOMAIN7:
    case BOOST_DOMAIN0:
    case BOOST_DOMAIN1:
    case BOOST_DOMAIN2:
    case BOOST_DOMAIN3:
    case BOOST_DOMAIN4:
    case BOOST_DOMAIN5:
    case BOOST_DOMAIN6:
    case BOOST_DOMAIN7:
      // Only enable the domain if it is disabled to avoid fbVoltage glitches
      if (sAnalog.domainStatus[domain].isEnabled != state)
      {
        Analog_selectChannel(domain, state);
        SELECT_DOMLEN();   // DOMLEN low to latch in new vals
        Time_delay(1);     // 1us to latch in the new value
        DESELECT_DOMLEN(); // DOMLEN high so we can otherwise use the bus
        Time_delay(1);     // 1us to let the DOMLEN latch in
//        if (state == TRUE)
//          Time_delay(250); // 250us from EN to active, per datasheet
      }
      DAC_setVoltage(DAC_PORT1, fbVoltage);
      /***** This is a hack to ensure that the fbVoltage is reaching the selected domain *****/
      Analog_selectChannel(domain, FALSE);
      // If this is the first time enabling the domain, give it some time for voltage to stabilize
      if ((sAnalog.domainStatus[domain].isEnabled != state) && (state == TRUE))
        Time_delay(250);  // 250us to stabilize voltage (datasheet says 500us...)

      // Make a record of what we just did here
      sAnalog.domainStatus[domain].isEnabled       = state;
      sAnalog.domainStatus[domain].domainVoltage   = vOut;
      sAnalog.domainStatus[domain].feedbackVoltage = fbVoltage;
      break;
  }
  // DOMEN needs to be low in order for fbVoltage to flow into the domain
  // DOMEN needs to be high before selecting another channel such that you don't unintentionally
  //       change the voltage on either the first or second domain
  // DOMEN is used to set the state of the D-latches which enable or disable each domain. This
  //       value is stored via DOMLEN high to low

  // Therefore, each domain would ideally operate as designed, with the analog module sweeping
  // through each domain refreshing the feedback voltage as it goes, but the above hack is
  // necessary because that isn't implemented yet.

  return TRUE;
}

/*************************************************************************************************\
* FUNCTION    Analog_getFeedbackVoltage
* DESCRIPTION Finds the appropriate feedback voltage for a given domain and desired domain voltage
* PARAMETERS  domain: The domain to enable or disable
              vDomain: The voltage to put on the selected domain
* RETURNS     The feedback voltage corresponding to vDomain on the desired domain
* NOTES       None
\*************************************************************************************************/
static double Analog_getFeedbackVoltage(VoltageDomain domain, double vDomain)
{
  double r1 = sAnalog.domainConfig[domain].r1,
         r2 = sAnalog.domainConfig[domain].r2,
         rf = sAnalog.domainConfig[domain].rf,
         vRef = Analog_getReferenceVoltage(sAnalog.domainConfig[domain].regulator);
  return (rf * (vRef * (1/r1 + 1/r2 + 1/rf) - vDomain * (1/r1)));
}

/*************************************************************************************************\
* FUNCTION    Analog_getOutputVoltage
* DESCRIPTION Finds the ideal output voltage for a given domain based on the input feedback voltage
* PARAMETERS  domain: The domain to enable or disable
              vFeedback: The voltage to put on the selected domain
* RETURNS     The ideal output voltage corresponding to vFeedback on the desired domain
* NOTES       None
\*************************************************************************************************/
static double Analog_getOutputVoltage(VoltageDomain domain, double vFeedback)
{
  double r1 = sAnalog.domainConfig[domain].r1,
         r2 = sAnalog.domainConfig[domain].r2,
         rf = sAnalog.domainConfig[domain].rf,
         vRef = Analog_getReferenceVoltage(sAnalog.domainConfig[domain].regulator);
  return vRef + (r1 * (vRef * (1/r2 + 1/rf) - (vFeedback/rf)));
}

/*************************************************************************************************\
* FUNCTION    Analog_sampleDomain
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  analogSelect: The domain to measure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_testAnalog(void)
{
  volatile double domainVoltage = Analog_getOutputVoltage(COMMS_DOMAIN, 0.65);
  /*
  uint8 bytesToSend = 0;
  uint16_t i, adcResult[5];
  float resolution, adcVolts[18];
  char adcResultChars[5][5], adcVoltsChars[5][5];
  FullDuplexPort *dataPort = UART_getPortPtr(UART_PORT5);
  
  Analog_selectChannel(analogSelect, 0);  // Select channels from the analog switches
  
  adcResult[0] = Main_getADC(1, 17); // Sample internal vRef (adc1 chan17)
  adcResult[1] = Main_getADC(1, 6);  // Sample external vRef (adc1 chan6)
  adcResult[2] = Main_getADC(1, 1);  // ADC1 Chan1
  adcResult[3] = Main_getADC(2, 2);  // ADC2 Chan2
  adcResult[4] = Main_getADC(3, 3);  // ADC3 Chan3
  resolution = 1.20000000 / (float)adcResult[0]; // External vRef
  
  while (!UART_isTxBufIdle(UART_PORT5));
  
  dataPort->tx.numTx = 0;
  dataPort->tx.txBuffer[bytesToSend++] = '\f';
  dataPort->tx.txBuffer[bytesToSend++] = 'S';
  dataPort->tx.txBuffer[bytesToSend++] = 'e';
  dataPort->tx.txBuffer[bytesToSend++] = 'l';
  dataPort->tx.txBuffer[bytesToSend++] = ':';
  dataPort->tx.txBuffer[bytesToSend++] = ' ';
  dataPort->tx.txBuffer[bytesToSend++] = analogSelect + 0x30;
  dataPort->tx.txBuffer[bytesToSend++] = '\r';
  dataPort->tx.txBuffer[bytesToSend++] = '\n';
  for (i=0; i<5; i++)
  {
    adcResultChars[i][4] = ((adcResult[i] % 10     ) / 1     ) + 0x30;
    adcResultChars[i][3] = ((adcResult[i] % 100    ) / 10    ) + 0x30;
    adcResultChars[i][2] = ((adcResult[i] % 1000   ) / 100   ) + 0x30;
    adcResultChars[i][1] = ((adcResult[i] % 10000  ) / 1000  ) + 0x30;
    adcResultChars[i][0] = ((adcResult[i] % 100000 ) / 10000 ) + 0x30;
    
    adcVolts[i] = adcResult[i] * resolution * 10000;
    dataPort->tx.txBuffer[bytesToSend++] = i + 0x30;
    dataPort->tx.txBuffer[bytesToSend++] = ':';
    dataPort->tx.txBuffer[bytesToSend++] = ' ';
    dataPort->tx.txBuffer[bytesToSend++] = adcResultChars[i][0];
    dataPort->tx.txBuffer[bytesToSend++] = adcResultChars[i][1];
    dataPort->tx.txBuffer[bytesToSend++] = adcResultChars[i][2];
    dataPort->tx.txBuffer[bytesToSend++] = adcResultChars[i][3];
    dataPort->tx.txBuffer[bytesToSend++] = adcResultChars[i][4];
    dataPort->tx.txBuffer[bytesToSend++] = ' ';
    dataPort->tx.txBuffer[bytesToSend++] = '=';
    dataPort->tx.txBuffer[bytesToSend++] = ' ';
      
    adcVoltsChars[i][4] = (((uint16_t)adcVolts[i] % 10     ) /  1    ) + 0x30;
    adcVoltsChars[i][3] = (((uint16_t)adcVolts[i] % 100    ) / 10    ) + 0x30;
    adcVoltsChars[i][2] = (((uint16_t)adcVolts[i] % 1000   ) / 100   ) + 0x30;
    adcVoltsChars[i][1] = (((uint16_t)adcVolts[i] % 10000  ) / 1000  ) + 0x30;
    adcVoltsChars[i][0] = (((uint16_t)adcVolts[i] % 100000 ) / 10000 ) + 0x30;
    
    dataPort->tx.txBuffer[bytesToSend++] = adcVoltsChars[i][0];
    dataPort->tx.txBuffer[bytesToSend++] = '.';
    dataPort->tx.txBuffer[bytesToSend++] = adcVoltsChars[i][1];
    dataPort->tx.txBuffer[bytesToSend++] = adcVoltsChars[i][2];
    dataPort->tx.txBuffer[bytesToSend++] = adcVoltsChars[i][3];
    dataPort->tx.txBuffer[bytesToSend++] = adcVoltsChars[i][4];
    dataPort->tx.txBuffer[bytesToSend++] = 'v';
    dataPort->tx.txBuffer[bytesToSend++] = '\r';
    dataPort->tx.txBuffer[bytesToSend++] = '\n';
  }
  UART_sendData(UART_PORT5, &dataPort->txBuffer[0], bytesToSend);
*/
}
