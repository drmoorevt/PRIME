#ifndef ANALOG_H
#define ANALOG_H

#include "types.h"

typedef enum 
{
  MCU_DOMAIN    = 0,
  ANALOG_DOMAIN = 1,
  SRAM_DOMAIN   = 2,
  SPI_DOMAIN    = 3,
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
  BOOST_DOMAIN7 = 15,
  NUM_ANALOG_DOMAINS
} VoltageDomain;

typedef enum
{
  TPS62240 = 0,
  NUM_REGULATORS
} RegulatorType;

typedef struct
{
  boolean isEnabled;
  float fbOutputVoltage;
  float domainVoltage;
} DomainStatus;

void Analog_init(void);
void Analog_setup(RegulatorType reg, double r1, double r2, double rf);
void Analog_sampleDomain(VoltageDomain analogSelect);
void Analog_setDomain(VoltageDomain domain, boolean state);
void Analog_selectChannel(VoltageDomain chan, boolean domen);
void Analog_adjustFeedbackVoltage(VoltageDomain domain, float voltage);
float Analog_convertFeedbackVoltage(VoltageDomain domain, float outVolts);

#endif // ANALOG_H
