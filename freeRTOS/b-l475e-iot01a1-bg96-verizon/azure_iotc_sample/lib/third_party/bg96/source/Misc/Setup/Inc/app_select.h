/**
  ******************************************************************************
  * @file    app_select.h
  * @author  MCD Application Team
  * @brief   Header for app_select.c module
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
#ifndef APP_SELECT_H
#define APP_SELECT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Application Codes */
typedef enum
{
  APP_SELECT_DEFAULT_APPLI    = 1U,
  APP_SELECT_SETUP_CODE       = 100U,
  APP_SELECT_CST_MODEM_START  = 101U,
} app_select_code_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
typedef uint8_t AS_CHART_t ;

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* init function of app_select component */
void app_select_init(void);

/* allows to record an application */
int32_t app_select_record(AS_CHART_t *app_label, uint16_t app_code);

/* menu display  */
uint16_t app_select_menu_start(void);


#ifdef __cplusplus
}
#endif

#endif /* APP_SELECT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
