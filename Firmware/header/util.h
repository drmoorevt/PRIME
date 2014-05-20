#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#define CCITT_CRC_POLYNOMIAL (0x1021)
#define CCITT_REV_POLYNOMIAL (0x8408)
#define CCITT_REVREP_POLYNOMIAL (0x8810)

void   Util_uint16ToASCII(uint16 inVal, char *outChars);
void   Util_swap16(uint16* pToSwap);
void   Util_swap32(uint32* pToSwap);
int8   Util_compareMemory(uint8* pLeft, uint8* pRight, uint16 numBytes);
void   Util_fillMemory(void *pDst, uint32 numBytes, uint8 fillVal);
void   Util_copyMemory(uint8* pSrc, uint8* pDst, uint16 numBytes);
void   Util_spinWait(volatile uint32 spinVal);
void   Util_spinDelay(uint32 microSecondDelay);

#endif // UTIL_H
