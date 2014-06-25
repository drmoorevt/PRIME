#include "util.h"

#define FILE_ID UTIL_C


#define BITMASK(X) (1L << (X))
/**************************************************************************************************\
* FUNCTION     Util_spinWait
* DESCRIPTION  Returns the inWord with the bottom nbrToReflect bits reflected
* PARAMETERS   inWord: The word to reflect
*              nbrToReflect: The number of bits in inWord (from the bottom) to reflect
* RETURNS      Nothing
* NOTES        Example: reflect(0x3e23L,3) == 0x3e26
\**************************************************************************************************/
uint32 Util_reflect(uint32 inWord, uint32 nbrToReflect)
{
 uint32 i, t = inWord;
 for (i = 0; i < nbrToReflect; i++)
   {
    if (t & 1L)
      inWord |=  BITMASK((nbrToReflect - 1) - i);
    else
      inWord &= ~BITMASK((nbrToReflect - 1) - i);
    t >>= 1;
   }
 return inWord;
}

/**************************************************************************************************\
* FUNCTION     Util_spinWait
* DESCRIPTION  Spins in a loop the number of times provided
* PARAMETERS   spinVal: The number of iterations to spin
* RETURNS      Nothing
* NOTES        None
\**************************************************************************************************/
void Util_spinWait(volatile uint32 spinVal)
{
  while (spinVal > 0)
    spinVal--;
} // Util_spinWait

/**************************************************************************************************\
* FUNCTION     Util_spinDelay
* DESCRIPTION  Spins in a loop for approximately the duration of the specified delay
* PARAMETERS   microSecondDelay: The minimum number of microseconds for which to delay
* RETURNS      Nothing
* NOTES        None
\**************************************************************************************************/
void Util_spinDelay(uint32 microSecondDelay)
{
  Util_spinWait((SystemCoreClock / 1000000) * microSecondDelay);
} // Util_spinWait

/**************************************************************************************************\
* FUNCTION     Util_checksum
* DESCRIPTION  Computes the 8bit checksum of pData. The checksum is the sum of all the data bytes,
*              negate the result and add 1.
* PARAMETERS   pData - pointer to data
*              nbrBytes - length of the data
* RETURNS      Checksum of the data
* NOTES        None
\**************************************************************************************************/
uint8 Util_checksum(const uint8 *pData, uint8 nbrBytes)
{
  uint8 sum = 0;
  uint8 i;

  for (i = 0; i < nbrBytes; i++)
    sum += pData[i];

  sum = ~sum + 1;
  return sum;
}  // Util_checksum

/**************************************************************************************************\
* FUNCTION     Util_compareMemory
* DESCRIPTION  Little endian comparison of two memory locations
* PARAMETERS   pLeft: The left value to compare
*              pRight: The right value to compare
*              numBytes: The number of bytes to compare from both values
* RETURNS      -1 if pLeft is greater than pRight
*               1 if pLeft is less than pRight
*               0 if the two values are equal
* NOTES        None
\**************************************************************************************************/
int8 Util_compareMemory(uint8* pLeft, uint8* pRight, uint16 numBytes)
{
  int32 i;
  for (i = numBytes - 1; i >= 0; i--) // Assuming little endian (MSB at highest addr)
  {
    if (pLeft[i] > pRight[i])
      return -1;
    else if (pLeft[i] < pRight[i])
      return 1;
  }
  return 0;
}

/**************************************************************************************************\
* FUNCTION     Util_fillMemory
* DESCRIPTION  Fills the provided buffer with the fillVal
* PARAMETERS   pDst: The destination buffer to fill
*              numBytes: Number of bytes to write
*              fillVal: The value to write to the buffer
* RETURNS      Nothing
* NOTES        None
\**************************************************************************************************/
void Util_fillMemory(void *pDst, uint32 numBytes, uint8 fillVal)
{
  uint8 *pDstAsBytes = (uint8*)pDst;
  while (numBytes-- > 0)
    pDstAsBytes[numBytes] = fillVal;
}

/**************************************************************************************************\
* FUNCTION     Util_copyMemory
* DESCRIPTION  Copies a number of bytes from one buffer to another
* PARAMETERS   pSrc: The source buffer
*              pDst: The destination buffer
*              numBytes: The number of bytes to copy from source to destination
* RETURNS      Nothing
* NOTES        None
\**************************************************************************************************/
void Util_copyMemory(uint8 *pSrc, uint8 *pDst, uint16 numBytes)
{
  while (numBytes-- > 0)
    pDst[numBytes] = pSrc[numBytes];
}

/**************************************************************************************************\
* FUNCTION     Util_swap16
* DESCRIPTION  Swaps the upper and lower byte of a uint16
* PARAMETERS   pToSwap: pointer to the LSB of the word to swap
* RETURNS      Nothing
* NOTES        Used for changing endianness
\**************************************************************************************************/
void Util_swap16(uint16 *pToSwap)
{
  uint16 temp = (*pToSwap >> 8) + (*pToSwap << 8);
  *pToSwap = temp;
}

/**************************************************************************************************\
* FUNCTION     Util_swap32
* DESCRIPTION  Swaps the upper and lower byte of a uint16
* PARAMETERS   pToSwap: pointer to the LSB of the word to swap
* RETURNS      Nothing
* NOTES        Used for changing endianness
\**************************************************************************************************/
void Util_swap32(uint32 *pToSwap)
{
  uint8 *pAsBytes = (uint8 *)pToSwap;
  uint32 temp = pAsBytes[3] + (pAsBytes[2] << 8) + (pAsBytes[1] << 16) + (pAsBytes[0] << 24);
  *pToSwap = temp;
}

/**************************************************************************************************\
* FUNCTION     Util_uint16ToASCII
* DESCRIPTION  Converts a uint16 to a 5-byte character array
* PARAMETERS   inVal: The value to convert
*              outChars: The uint16 as an array of characters
* RETURNS      Nothing
* NOTES        None
\**************************************************************************************************/
void Util_uint16ToASCII(uint16 inVal, char *outChars)
{
  const char lookup[] = { '0', '1', '2','3','4','5','6','7','8','9' };
  outChars[0] = lookup[(inVal / 10000)    ];
  outChars[1] = lookup[(inVal / 1000) % 10];
  outChars[2] = lookup[(inVal / 100)  % 10];
  outChars[3] = lookup[(inVal / 10)   % 10];
  outChars[4] = lookup[(inVal / 1)    % 10];
}
