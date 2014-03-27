#include "stm32f2xx.h"
#include "adc.h"
#include "analog.h"
#include "dac.h"
#include "eeprom.h"
#include "gpio.h"
#include "uart.h"
#include "util.h"

#define FILE_ID ANALOG_C

union
{
  struct
  {
    uint32_t domen   : 1;
    uint32_t domsel1 : 1;
    uint32_t domsel2 : 1;
    uint32_t domsel0 : 1;
    uint32_t domsel3 : 1;
    uint32_t domrst  : 1;
    uint32_t filler  : 26;
  } asBfldLH;
  uint32_t asWord;
  uint8_t  asBytes[4];
} portCLSB;

typedef struct
{
  boolean isEnabled;
  float voltSetting;
  float voltActual;
} DomainStatus;


struct
{
  DomainStatus domain[NUM_ANALOG_DOMAINS];
} sAnalog;

#define SELECT_DOMLEN()   do { GPIOE->BSRRH |= 0x00000004; } while (0)
#define DESELECT_DOMLEN() do { GPIOE->BSRRL |= 0x00000004; } while (0)

/*************************************************************************************************\
* FUNCTION    Analog_init
* DESCRIPTION Initializes the pins and data structures required for analog measurement and control
* PARAMETERS  None
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_init(void)
{
  const uint16 ctrlPortB = GPIO_Pin_0;
  const uint16 ctrlPortC = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
  const uint16 ctrlPortE = GPIO_Pin_2;

  GPIO_InitTypeDef analogCtrlPortB = {ctrlPortB, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef analogCtrlPortC = {ctrlPortC, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_InitTypeDef analogCtrlPortE = {ctrlPortE, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP,
                                                 GPIO_PuPd_NOPULL, GPIO_AF_SYSTEM };
  GPIO_setPortClock(GPIOB, TRUE);
  GPIO_configurePins(GPIOB, &analogCtrlPortB);

  GPIO_setPortClock(GPIOC, TRUE);
  GPIO_configurePins(GPIOC, &analogCtrlPortC);

  GPIO_setPortClock(GPIOE, TRUE);
  GPIO_configurePins(GPIOE, &analogCtrlPortE);
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
  portCLSB.asWord = 0;
  portCLSB.asBfldLH.domen   = (domen == FALSE)          ? 0 : 1;
  portCLSB.asBfldLH.domsel0 = ((chan & 0x00000001) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel1 = ((chan & 0x00000002) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel2 = ((chan & 0x00000004) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domsel3 = ((chan & 0x00000008) > 0) ? 1 : 0;
  portCLSB.asBfldLH.domrst  = (1);  // keep domrst high so we dont latch->demux the 259s
  GPIOC->ODR = (GPIOC->ODR & 0xFFC0) | (portCLSB.asBytes[0]);
}

/*************************************************************************************************\
* FUNCTION    Analog_setDomain
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  domain: The domain to enable or disable
              state: The state in which to put the selected domain
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_setDomain(VoltageDomain domain, boolean state)
{
  switch (domain)
  {
    case MCU_DOMAIN:
      break; // Can't enable or disable the MUC domain
    case ANALOG_DOMAIN:  // V1EN is connected to a pin PB0
      if (state)
        GPIOB->BSRRL = 0x0001;
      else
        GPIOB->BSRRH = 0x0001;
      break;
    case SRAM_DOMAIN:
    case EEPROM_DOMAIN:
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
      Analog_selectChannel(domain, state);
      SELECT_DOMLEN();        // DOMLEN low to latch in new vals
      Util_spinWait(12000);   // 100us to latch in the new value
      DESELECT_DOMLEN();      // DOMLEN high so we can otherwise use the bus
      Util_spinWait(12000);   // 100us to let the DOMLEN latch in
      break;
  }
}

/*************************************************************************************************\
* FUNCTION    Analog_adjustDomain
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  domain: The domain to enable or disable
              voltage: The voltage to put on the selected domain
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_adjustDomain(VoltageDomain domain, float voltage)
{ 
  switch (domain)
  {
    case MCU_DOMAIN:
    case ANALOG_DOMAIN:
    case SRAM_DOMAIN:
    case EEPROM_DOMAIN:
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
      break;
  }
  
  DAC_setVoltage(DAC_PORT1, voltage);
  Analog_selectChannel(domain, FALSE);  // Charge up the voltage capacitor (domen active low)
//  Analog_selectChannel(domain, TRUE);   // Turn on the inhibit switch
}

float Analog_convertFeedbackVoltage(VoltageDomain domain, float outVolts)
{
  switch (domain)
  {
    case MCU_DOMAIN:
    case ANALOG_DOMAIN:
    case SRAM_DOMAIN:
      break;
    case EEPROM_DOMAIN:
      return 0.6 + (3.400 - outVolts)/2.4000;
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
      break;
  }
  return 0.6;
}

/*************************************************************************************************\
* FUNCTION    Analog_sampleDomain
* DESCRIPTION Enables or disables a voltage domain
* PARAMETERS  analogSelect: The domain to measure
* RETURNS     Nothing
* NOTES       None
\*************************************************************************************************/
void Analog_sampleDomain(VoltageDomain analogSelect)
{
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
  UART_sendData(UART_PORT5, bytesToSend);
*/
}
