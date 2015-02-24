#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "stm32f4xx.h"

#define FALSE 0
#define TRUE  1

#ifndef NULL
  #define NULL ((void *)0)
#endif

typedef uint8_t  boolean;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#endif
