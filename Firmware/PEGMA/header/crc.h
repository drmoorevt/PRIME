#ifndef CRC_H
#define CRC_H

#include "types.h"

typedef enum
{
  CRC16_POLY_CCITT_STD = 0,
  CRC16_POLY_CCITT_REV = 1,
  CRC16_POLY_CCITT_REC = 2,
  CRC16_POLY_ANSI_STD  = 3,
  CRC16_POLY_ANSI_REV  = 4,
  CRC16_POLY_ANSI_REC  = 5,
  CRC16_POLY_MAX       = 6,
} CRC16Polynomial;

typedef enum
{
  CRC7_SDCARD_POLY = 0
} CRC7Polynomial;

uint8  CRC_calcCRC7(CRC7Polynomial crcType, const uint8 *pData, uint16 numBytes);
uint16 CRC_crc16(uint16 initial, CRC16Polynomial crcType, uint8 *pData, uint32 numBytes);

#endif // CRC_H
