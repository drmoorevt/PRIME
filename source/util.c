#include "util.h"

#define FILE_ID UTIL_C

/*----------------------------------------------------------------------------
  wait function
 *----------------------------------------------------------------------------*/
void Util_spinWait(uint32 spinVal)
{
  while (spinVal-- > 0);
}

int8 Util_compareMemory(uint8* pLeft, uint8* pRight, uint8 numBytes)
{
  uint8 i;
  for (i = numBytes - 1; i > 0; i--)
  {
    if (pLeft[i] > pRight[i])
      return -1;
    else if (pLeft[i] < pRight[i])
      return 1;
  }
  return 0;
}

void Util_fillMemory(uint8* pDst, uint32 numBytes, uint8 fillVal)
{
  while (numBytes-- > 0)
    pDst[numBytes] = fillVal;
}

void Util_copyMemory(uint8* pSrc, uint8* pDst, uint16 numBytes)
{
  while (numBytes-- > 0)
    pDst[numBytes] = pSrc[numBytes];
}

void Util_swap16(uint16* pToSwap)
{
  uint16 temp = (*pToSwap>>8) + (*pToSwap<<8);
  *pToSwap = temp;
}

void Util_uint16ToASCII(uint16 inVal, char *outChars)
{
  const char lookup[] = { '0', '1', '2','3','4','5','6','7','8','9' };
  outChars[0] = lookup[(inVal / 10000)    ];
  outChars[1] = lookup[(inVal / 1000) % 10];
  outChars[2] = lookup[(inVal / 100)  % 10];
  outChars[3] = lookup[(inVal / 10)   % 10];
  outChars[4] = lookup[(inVal / 1)    % 10];
}
