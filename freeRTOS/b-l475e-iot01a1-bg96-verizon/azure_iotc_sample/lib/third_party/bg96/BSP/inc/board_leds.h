/**
  ******************************************************************************
  * @file    board_leds.h
  * @author  MCD Application Team
  * @brief   Header for board_leds.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      http://www.st.com/SLA0044
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOARD_LEDS_H
#define BOARD_LEDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l475e_iot01.h"  
/* Exported types ------------------------------------------------------------*/
/* Map BL_Led_t to Led_TypeDef from BSP */
typedef enum
{
  BL_LED1       = 0xFFFF,
  BL_LED2       = LED2,
  BL_LED_ORANGE = 0xFFFF,
  BL_LED_GREEN  = LED2
} BL_Led_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void BL_LED_Init(BL_Led_t led);
void BL_LED_Off(BL_Led_t led);
void BL_LED_On(BL_Led_t led);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_LEDS_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
