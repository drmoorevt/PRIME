#ifndef GPIO_H
#define GPIO_H

#include "types.h"

#define GPIO_Pin_0   ((uint16)0x0001)  /* Pin 0 selected */
#define GPIO_Pin_1   ((uint16)0x0002)  /* Pin 1 selected */
#define GPIO_Pin_2   ((uint16)0x0004)  /* Pin 2 selected */
#define GPIO_Pin_3   ((uint16)0x0008)  /* Pin 3 selected */
#define GPIO_Pin_4   ((uint16)0x0010)  /* Pin 4 selected */
#define GPIO_Pin_5   ((uint16)0x0020)  /* Pin 5 selected */
#define GPIO_Pin_6   ((uint16)0x0040)  /* Pin 6 selected */
#define GPIO_Pin_7   ((uint16)0x0080)  /* Pin 7 selected */
#define GPIO_Pin_8   ((uint16)0x0100)  /* Pin 8 selected */
#define GPIO_Pin_9   ((uint16)0x0200)  /* Pin 9 selected */
#define GPIO_Pin_10  ((uint16)0x0400)  /* Pin 10 selected */
#define GPIO_Pin_11  ((uint16)0x0800)  /* Pin 11 selected */
#define GPIO_Pin_12  ((uint16)0x1000)  /* Pin 12 selected */
#define GPIO_Pin_13  ((uint16)0x2000)  /* Pin 13 selected */
#define GPIO_Pin_14  ((uint16)0x4000)  /* Pin 14 selected */
#define GPIO_Pin_15  ((uint16)0x8000)  /* Pin 15 selected */
#define GPIO_Pin_All ((uint16)0xFFFF)  /* All pins selected */

typedef enum
{
  GPIO_Mode_IN   = 0x00,
  GPIO_Mode_OUT  = 0x01,
  GPIO_Mode_AF   = 0x02,
  GPIO_Mode_AN   = 0x03
} GPIOMode_TypeDef;

typedef enum
{
  GPIO_OType_PP = 0x00,
  GPIO_OType_OD = 0x01
} GPIOOType_TypeDef;

typedef enum
{
  GPIO_Speed_2MHz   = 0x00, /*!< Low speed */
  GPIO_Speed_25MHz  = 0x01, /*!< Medium speed */
  GPIO_Speed_50MHz  = 0x02, /*!< Fast speed */
  GPIO_Speed_100MHz = 0x03  /*!< High speed on 30 pF (80 MHz Output max speed on 15 pF) */
} GPIOSpeed_TypeDef;

typedef enum
{
  GPIO_PuPd_NOPULL = 0x00,
  GPIO_PuPd_UP     = 0x01,
  GPIO_PuPd_DOWN   = 0x02
} GPIOPuPd_TypeDef;

typedef struct
{
  uint32_t          GPIO_Pin;
  GPIOMode_TypeDef  GPIO_Mode;
  GPIOSpeed_TypeDef GPIO_Speed;
  GPIOOType_TypeDef GPIO_OType;
  GPIOPuPd_TypeDef  GPIO_PuPd;
} GPIO_InitTypeDef;

void GPIO_init(void);
void GPIO_structInitUART(GPIO_InitTypeDef *GPIO_InitStruct, uint16 pin);
boolean GPIO_setPortClock(GPIO_TypeDef *GPIOx, boolean clockState);
boolean GPIO_configurePins(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct);

#endif // GPIO_H
