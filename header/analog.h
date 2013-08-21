#ifndef ANALOG_H
#define ANALOG_H

#include "types.h"

typedef enum 
{
  MCU_DOMAIN    = 0,
  ANALOG_DOMAIN = 1,
  SRAM_DOMAIN   = 2,
  EEPROM_DOMAIN = 3,
  ENERGY_DOMAIN = 4,
  COMMS_DOMAIN  = 5,
  IO_DOMAIN     = 6,
  BUCK_DOMAIN7  = 7,
  BOOST_DOMAIN0 = 8,
  BOOST_DOMAIN1 = 9,
  BOOST_DOMAIN2 = 10,
  BOOST_DOMAIN3 = 11,
  BOOST_DOMAIN4 = 12,
  BOOST_DOMAIN5 = 13,
  BOOST_DOMAIN6 = 14,
  BOOST_DOMAIN7 = 15
} VoltageDomain;

void Analog_sampleDomain(VoltageDomain analogSelect);
void Analog_setDomain(VoltageDomain domain, boolean state);
void Analog_selectChannel(VoltageDomain chan, boolean domen);
void Analog_adjustDomain(VoltageDomain domain, float voltage);

#endif // ANALOG_H
