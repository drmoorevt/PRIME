#ifndef CRC_H
#define CRC_H

#include "types.h"

typedef enum
{
  CRC16_CCITT_POLY = 0,
  CRC16_IEEE_POLY  = 1,
} CRC16Polynomial;

typedef enum
{
  CRC7_SDCARD_POLY = 0
} CRC7Polynomial;

uint8  CRC_calcCRC7(CRC7Polynomial crcType, const uint8 *pData, uint16 numBytes);
uint16 CRC_calcCRC16(uint16 initial, CRC16Polynomial crcType, const uint8 *pData, uint32 numBytes);

#endif // CRC_H
