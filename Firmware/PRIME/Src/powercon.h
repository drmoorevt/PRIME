#ifndef POWERCON_H
#define POWERCON_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef enum
{
  VOLTAGE_DOMAIN_0    = 0x00,   // static 3.3V
  VOLTAGE_DOMAIN_1    = 0x01,   // 3.3V, 2.7V, 2.3V, 1.8V
  VOLTAGE_DOMAIN_2    = 0x02,   // dynamic 3.3-->1.2V
  VOLTAGE_DOMAIN_NONE = 0x03,   // No domain selected (disconnect peripheral device)
  VOLTAGE_DOMAIN_MAX  = 0x04    
} VoltageDomain;

typedef enum
{
  DEVICE_EEPROM    = 0x00,
  DEVICE_NORFLASH  = 0x01,
  DEVICE_NANDFLASH = 0x02,
  DEVICE_SDCARD    = 0x03,
  DEVICE_PROXSENSE = 0x04,
  DEVICE_TEMPSENSE = 0x05,
  DEVICE_WIFIMOD   = 0x06,
  DEVICE_BTMOD     = 0x07,
  DEVICE_MAX       = 0x08
} Device;

bool PowerCon_init(DAC_HandleTypeDef *pDAC);
bool PowerCon_setDomainVoltage(VoltageDomain dom, double voltage);
bool PowerCon_setDeviceDomain(Device dev, VoltageDomain dom);

bool PowerCon_powerSupplyPOST(VoltageDomain domain);

#endif
