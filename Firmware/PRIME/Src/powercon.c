#include "analog.h"
#include "extusb.h"
#include "powercon.h"
#include <string.h>

#define FILE_ID POWERCON_C

static struct
{
  DAC_HandleTypeDef hdac;
  bool isConfigured;
} sPowerCon;

/**************************************************************************************************\
* FUNCTION    PowerCon_setDomainVoltage
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool PowerCon_setDomainVoltage(VoltageDomain dom, double voltage)
{
  const double r1 = 130000, r2 = 91000, rf = 220000, vref = 1.1;
  double vf;
  GPIO_PinState vAdj0, vAdj1;
  uint16_t dacVal;
  switch (dom)
  {
    case VOLTAGE_DOMAIN_1:
      if (voltage >= 3.3)  // Set 3.3V for any request (voltage >= to 3.3V)
      {
        vAdj0 = GPIO_PIN_RESET;
        vAdj1 = GPIO_PIN_RESET;
      }
      else if (voltage >= 2.7) // Set 2.7V for any request (3.3 >= voltage >= 2.7)
      {
        vAdj0 = GPIO_PIN_RESET;
        vAdj1 = GPIO_PIN_SET;
      }
      else if (voltage >= 2.3) // Set 2.3V for any request (2.7 >= voltage >= 2.3)
      {
        vAdj0 = GPIO_PIN_SET;
        vAdj1 = GPIO_PIN_RESET;
      }
      else // Set 1.8V for any request (voltage < 2.3)
      {
        vAdj0 = GPIO_PIN_SET;
        vAdj1 = GPIO_PIN_SET;
      }
      HAL_GPIO_WritePin(VADJ1_0_GPIO_Port, VADJ1_0_Pin, vAdj0);
      HAL_GPIO_WritePin(VADJ1_1_GPIO_Port, VADJ1_1_Pin, vAdj1);
      break;
    case VOLTAGE_DOMAIN_2:
      vf = rf * (vref * ((1/r2) + (1/rf)) - ((voltage - vref) / r1));
      dacVal = vf * (4095.0 / 3.3);
      HAL_DAC_SetValue(&sPowerCon.hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dacVal);
      HAL_DAC_Start(&sPowerCon.hdac, DAC_CHANNEL_2);
      break;
    default:
      return false;
  }
  return true;
}

/**************************************************************************************************\
* FUNCTION    PowerCon_setDeviceDomain
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool PowerCon_setDeviceDomain(Device dev, VoltageDomain dom)
{
  GPIO_PinState vSel0 = ((uint8_t)dev & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  GPIO_PinState vSel1 = ((uint8_t)dev & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  GPIO_PinState vSel2 = ((uint8_t)dev & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  
  // PVD flipped for PPS330D miswire
  GPIO_PinState pvd0  = ((uint8_t)dom & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  GPIO_PinState pvd1  = ((uint8_t)dom & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  
  // Setup Device Address
  HAL_GPIO_WritePin(PV_VSEL0_GPIO_Port, PV_VSEL0_Pin, vSel0);
  HAL_GPIO_WritePin(PV_VSEL1_GPIO_Port, PV_VSEL1_Pin, vSel1);
  HAL_GPIO_WritePin(PV_VSEL2_GPIO_Port, PV_VSEL2_Pin, vSel2);
  
  // Set Device Domain (latch in the new value)
  HAL_GPIO_WritePin(_PV_EN_GPIO_Port,  _PV_EN_Pin,  GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PV_D0_GPIO_Port, PV_D0_Pin, pvd0);
  HAL_GPIO_WritePin(PV_D1_GPIO_Port, PV_D1_Pin, pvd1);
  HAL_GPIO_WritePin(_PV_EN_GPIO_Port,  _PV_EN_Pin,  GPIO_PIN_SET);
  
  return true;
}

/**************************************************************************************************\
* FUNCTION    PowerCon_init
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool PowerCon_init(DAC_HandleTypeDef *pDAC)
{
  memset(&sPowerCon, 0x00, sizeof(sPowerCon));
  memcpy(&sPowerCon.hdac, pDAC, sizeof(sPowerCon.hdac));
  
  // Clear the peripheral voltage select pins
  HAL_GPIO_WritePin(_PV_EN_GPIO_Port,  _PV_EN_Pin,  GPIO_PIN_SET);
  HAL_GPIO_WritePin(_PV_CLR_GPIO_Port, _PV_CLR_Pin, GPIO_PIN_RESET);
  HAL_Delay(1);
  
  // Turn the 74HC259s into no-action state
  HAL_GPIO_WritePin(_PV_EN_GPIO_Port,  _PV_EN_Pin,  GPIO_PIN_SET);
  HAL_GPIO_WritePin(_PV_CLR_GPIO_Port, _PV_CLR_Pin, GPIO_PIN_SET);
  
  // Disconnect all peripheral devices
  Device i;
  for (i = DEVICE_EEPROM; i < DEVICE_MAX; i++)
    PowerCon_setDeviceDomain(i, VOLTAGE_DOMAIN_NONE);
  
  //Default voltage domains to 3.3, 2.7, 1.8
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 2.7);
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, 1.8);
  return true;
}

/**************************************************************************************************\
* FUNCTION    PowerCon_powerSupplyPOST
* DESCRIPTION 
* PARAMETERS  
* RETURNS     
\**************************************************************************************************/
bool PowerCon_powerSupplyPOST(VoltageDomain domain)
{
  ADCSelect chanNum;
  char outBuf[64];
  uint32_t adcVal, outLen, counter = 0;
  switch (counter)
  {
    case 0:    PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 3.3); break;
    case 1023: PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 2.7); break;
    case 2047: PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 2.3); break;
    case 3071: PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_1, 1.8); break;
    case 4095: counter = 0;                                      return true;
  }
  PowerCon_setDomainVoltage(VOLTAGE_DOMAIN_2, counter * 3.3 / 4095.0);
  
  for (chanNum = ADC_VREF_INTERNAL; chanNum < ADC_SELECT_MAX; chanNum++)
  {
    Analog_getADCVal(chanNum);
    outLen = sprintf(outBuf, "%d ", adcVal);
    //ExtUSB_tx((uint8_t *)outBuf, outLen);
  }
  outLen = sprintf(outBuf, "\r\n");
  //ExtUSB_tx((uint8_t *)outBuf, outLen);
  return true;
}
