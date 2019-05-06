/**
  ******************************************************************************
  * @file    app_select.c
  * @author  MCD Application Team
  * @brief   application selection at boot
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

/* Includes ------------------------------------------------------------------*/
#include "app_select.h"
#include "plf_config.h"
#include <stdint.h>
#include "menu_utils.h"
#include "error_handler.h"

/* Private defines -----------------------------------------------------------*/

#define APP_SELECT_VERSION  "1.0.0_IoT-DK"
#define APP_SELECT_DATE     "2018/06/29"

/* Number max of application recorded  */
#define APP_SELECT_NUMBER_MAX     10U

/* Timeout to display menu before to start the firmware */
#define APP_SELECT_TIMEOUT      4000U



/* Private macros ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/

/* data structure of recorded applications */
typedef struct
{
  uint8_t    *label;
  uint16_t    code;
} app_select_list_t;

/* Private variables ---------------------------------------------------------*/

/* Table of recorded applications */
static app_select_list_t app_select_list[APP_SELECT_NUMBER_MAX];


static AS_CHART_t *app_start_label = (AS_CHART_t *)"Start";

/* number of recorded applications */
static uint16_t app_select_nb = 0U;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  component init
  * @param  none
  * @retval none
  */
void app_select_init(void)
{
#if defined ( __GNUC__ )
  /* For SW4STM32-Eclipe only. This is needed to avoid problems when using printf(). */
  setbuf(stdout, NULL);
#endif /* __GNUC__ */
  app_select_nb = 0U;
  app_select_record(app_start_label, APP_SELECT_DEFAULT_APPLI);

}

/* allows to record an application */
int32_t app_select_record(AS_CHART_t *app_label, uint16_t app_code)
{
  int32_t ret;
  if (app_select_nb >= APP_SELECT_NUMBER_MAX)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 1, ERROR_WARNING);
    ret = -1;
  }
  else
  {
    app_select_list[app_select_nb].label = app_label;
    app_select_list[app_select_nb].code  = app_code;
    app_select_nb++;
    ret = 0;
  }
  return ret;
}

/**
  * @brief  component start -  display menu
  * @param  none
  * @retval application to start
  */
uint16_t app_select_menu_start(void)
{
  uint16_t i;
  uint16_t ret;
  int32_t ret_car;
  uint8_t car;
  uint8_t num_appli;

  ret = APP_SELECT_DEFAULT_APPLI;

  PrintSetup("\n\r\n\r");
  PrintSetup("=============================\n\r");
  PrintSetup("    STM32 CELLULAR\n\r");
  PrintSetup("      Version: %s\n\r", APP_SELECT_VERSION);
  PrintSetup("      Date:    %s\n\r", APP_SELECT_DATE);
  PrintSetup("=============================\n\r");

  PrintSetup("Select the application to run:\n\r\n\r");

  for (i = (uint16_t)0; i < app_select_nb; i++)
  {
    PrintSetup("%d: %s\n\r", i, app_select_list[i].label);
  }

  ret_car =  menu_utils_get_char_timeout(&car, APP_SELECT_TIMEOUT);
  if (ret_car)
  {
    num_appli = car - 0x30U;
    if ((num_appli != 0U) && ((uint16_t)num_appli < app_select_nb))
    {
      ret = app_select_list[num_appli].code;
    }
  }
  else
  {
    /* application code not match : starting firmware by default */
    ret = APP_SELECT_DEFAULT_APPLI;
  }
  return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
