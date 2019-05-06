/**
  ******************************************************************************
  * @file    menu_utils.h
  * @author  MCD Application Team
  * @brief   Header for menu_utils.c module
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
#ifndef MENU_UTILS_H
#define MENU_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "plf_config.h"

/* Exported constants --------------------------------------------------------*/

/*  MENU_UTILS_SETUP_CONFIG_SIZE_MAX can be increased to FLASH_PAGE_SIZE if necessary */
#define MENU_UTILS_SETUP_CONFIG_SIZE_MAX 512U

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  MENU_SETUP_SOURCE_NONE = 0,
  MENU_SETUP_SOURCE_UART,
  MENU_SETUP_SOURCE_DEFAULT,
  MENU_SETUP_SOURCE_FEEPROM,
  MENU_SETUP_SOURCE_MAX
} menu_setup_source_t;

typedef unsigned char MU_CHAR_t ;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintSetup(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, format, ## args)
#else
#define PrintSetup(format, args...)  printf("" format, ## args)
#endif /* USE_PRINTF */

/* Exported functions ------------------------------------------------------- */

/* get a char from the console uart  */
int32_t menu_utils_get_uart_char(uint8_t *car);

/* get a char from uart with timeout  */
int32_t menu_utils_get_char_timeout(uint8_t *car, uint32_t timeout);

/* set the new configuration to parse  */
void menu_utils_get_new_config(uint8_t **config_addr, uint32_t *config_size);

/* get the next line of the current config source  */
uint32_t  menu_utils_get_line(uint8_t string[], uint32_t size);

/* get and analyse the next line of the current config source  */
void menu_utils_get_string(MU_CHAR_t *title, uint8_t string[], uint32_t size);

/* set the source of new configuration to parse  */
void menu_utils_set_source(menu_setup_source_t type, uint8_t *config, uint32_t config_size);


#ifdef __cplusplus
}
#endif

#endif /* MENU_UTILS_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

