#ifndef UTIL_H
#define UTIL_H

#include "types.h"

void Util_uint16ToASCII(uint16 inVal, char *outChars);
void Util_swap16(uint16* pToSwap);
int8 Util_compareMemory(uint8* pLeft, uint8* pRight, uint8 numBytes);
void Util_fillMemory(uint8* pDst, uint32 numBytes, uint8 fillVal);
void Util_copyMemory(uint8* pSrc, uint8* pDst, uint16 numBytes);
void Util_spinWait(uint32 spinVal);

#endif // UART_H
