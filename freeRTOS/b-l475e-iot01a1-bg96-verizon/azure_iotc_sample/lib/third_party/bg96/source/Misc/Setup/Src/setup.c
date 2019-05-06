/**
  ******************************************************************************
  * @file    setup.c
  * @author  MCD Application Team
  * @brief   setup configuration management
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
#include "setup.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plf_sw_config.h"
#include "app_select.h"
#include "menu_utils.h"
#if (FEEPROM_UTILS_FLASH_USED == 1)
#include "feeprom_utils.h"
#endif
#include "time_date.h"

/* Private defines -----------------------------------------------------------*/
#define SETUP_NUMBER_MAX   10U
#define SETUP_APP_SELECT_LABEL "Setup Menu"

/* Private macros ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* list of actions required by setup menu */
typedef enum
{
  SETUP_ACTION_NONE = 0,
  SETUP_ACTION_UART,
  SETUP_ACTION_FEEPROM,
  SETUP_ACTION_ERASE,
  SETUP_ACTION_DUMP,
  SETUP_ACTION_QUIT
} setup_action_t;

/* list of configuration sources */
static int8_t *setup_source_string[MENU_SETUP_SOURCE_MAX] =
{
  (int8_t *)"NONE",
  (int8_t *)"UART",
  (int8_t *)"DEFAULT",
  (int8_t *)"FEEPROM"
};


/* setup application parameters */
typedef struct
{
  setup_appli_code_t code_appli;
  setup_appli_version_t version_appli;
  MU_CHAR_t         *label_appli;
  void (*setup_fnct)(void) ;
  void (*dump_fnct)(void) ;
  MU_CHAR_t        **default_config ;
  uint32_t           default_config_size ;
  uint8_t            config_done ;
} setup_params_t;


/* Private variables ---------------------------------------------------------*/

/* table of setup parameters for each application */
static setup_params_t setup_list[SETUP_NUMBER_MAX];

/* Number of application using setup */
static uint16_t setup_nb = 0U;

/* default configuration buffer */
static uint8_t setup_default_config[MENU_UTILS_SETUP_CONFIG_SIZE_MAX];

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


/* static function declarations */
static void setup_get_source(void);
static uint32_t setup_default_config_set(uint8_t **default_config, uint32_t default_config_size);
static uint32_t setup_feeprom_config_set(setup_appli_code_t code_appli, setup_appli_version_t version_appli, uint8_t **config_addr);
static setup_action_t setup_get_action(MU_CHAR_t *label);
static int32_t setup_flash_erase(setup_appli_code_t appli_code, setup_appli_version_t version_appli);
static void setup_timedate_handle(uint32_t num_appli);
static void setup_handle(uint32_t num_appli);


/* Functions Definition ------------------------------------------------------*/

/* select setup configuration source between flash (if valid) location and default (compilation values) */
static void setup_get_source(void)
{
  uint16_t i;
  menu_setup_source_t setup_source;

  PrintSetup("\r\n---------------------------------\n\r");
  PrintSetup("   List of config sources\n\r");
  PrintSetup("---------------------------------\n\r");

  for (i = 0U; i < setup_nb; i++)
  {
#if (FEEPROM_UTILS_FLASH_USED == 1)
    uint32_t config_size = 0U;
    uint8_t *config_addr;
    if (feeprom_utils_read_config_flash(setup_list[i].code_appli, setup_list[i].version_appli, &config_addr, &config_size) == 0)
    {
      setup_source = MENU_SETUP_SOURCE_FEEPROM;
    }
    else
#endif
    {
      setup_source = MENU_SETUP_SOURCE_DEFAULT;
    }
    PrintSetup(" %s Config from %s \n\r", setup_list[i].label_appli, setup_source_string[setup_source]);
  }
}

/* set setup configuration with defaylt values */
static uint32_t setup_default_config_set(uint8_t **default_config, uint32_t default_config_size)
{
  uint32_t config_index = 0U;
  uint32_t  i;

  for (i = 0U ; i < default_config_size; i++)
  {
    if (config_index + strlen((char *)default_config[i]) + 1U >= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
    {
      PrintSetup("SETUP Warning : config too large\r\n");
    }
    else
    {
      strcpy((char *)&setup_default_config[config_index], (char *)default_config[i]);
      config_index += strlen((char *)default_config[i]);
      setup_default_config[config_index] = '\r';
      config_index++;
    }
  }
  return config_index;
}

/* select from menu the setup action to process  */
static setup_action_t setup_get_action(MU_CHAR_t *label)
{
  setup_action_t sel = SETUP_ACTION_NONE;
  int32_t  ret;
  uint8_t  car = 0U;

  PrintSetup("\n\r");
  PrintSetup("---------------------------------\n\r");
  PrintSetup("   %s - %s  \n\r", SETUP_APP_SELECT_LABEL, label);
  PrintSetup("---------------------------------\n\r");

#if (FEEPROM_UTILS_FLASH_USED == 1)
  /*  PrintINFO(" f : get flash config(default choice)\n\r"); */
#endif
  PrintSetup(" c : set config by console\n\r");
  /*  PrintINFO(" d : default config\n\r"); */
#if (FEEPROM_UTILS_FLASH_USED == 1)
  PrintSetup(" e : erase config in flash (restore default)\n\r");
#endif
  PrintSetup(" l : list config \n\r");
  PrintSetup(" q : quit \n\r");

  ret = menu_utils_get_uart_char(&car);
  if (ret)
  {
    switch (car)
    {
      case 'c':
      case 'C':
      {
        /* update setup configuration from uart console */
        sel = SETUP_ACTION_UART;
        break;
      }
      case 'e':
      case 'E':
      {
        /* erase flash setup configuration */
        sel = SETUP_ACTION_ERASE;
        break;
      }
      case 'l':
      case 'L':
      {
        /* dump current active setup configuration */
        sel = SETUP_ACTION_DUMP;
        break;
      }
      case 'q':
      case 'Q':
      {
        /* cancel : return from this menu */
        sel = SETUP_ACTION_QUIT;
        break;
      }
      default:
      {
        sel = SETUP_ACTION_QUIT;
        break;
      }
    }
  }
  return sel;
}

/* set current config from flash  */
static uint32_t setup_feeprom_config_set(setup_appli_code_t code_appli, setup_appli_version_t version_appli, uint8_t **config_addr)
{
  uint32_t config_size = 0U;
#if (FEEPROM_UTILS_FLASH_USED == 1)
  feeprom_utils_read_config_flash(code_appli, version_appli, config_addr, &config_size);
#endif
  return config_size;
}


/* erase a setup configuration in flash  */
static int32_t setup_flash_erase(setup_appli_code_t appli_code, setup_appli_version_t version_appli)
{
  int32_t ret = -1;
#if (FEEPROM_UTILS_FLASH_USED == 1)
  uint32_t page_number;
  uint32_t flash_addr;

  ret = feeprom_utils_find_bank(appli_code, version_appli, &flash_addr, &page_number);
  if (ret >= 0)
  {
    ret = feeprom_utils_flash_erase(page_number);
  }
#endif
  return ret;
}

/* particular case to update date and time system   */
static void setup_timedate_handle(uint32_t num_appli)
{
  menu_utils_set_source(MENU_SETUP_SOURCE_UART, NULL, 0U);
  timedate_setup_handler();
}

/* processing of selected setup action   */
static void setup_handle(uint32_t num_appli)
{
  uint32_t setup_config_size = 0U;
  uint8_t *setup_config_addr = NULL;
  setup_action_t setup_action;
  menu_setup_source_t setup_source;
  uint8_t string_version[5];
  uint32_t bad_version ;
  uint32_t size ;

  /* get action to process from setup menu */
  setup_action = setup_get_action(setup_list[num_appli].label_appli);

  if (setup_action != SETUP_ACTION_QUIT)
  {
    if (SETUP_ACTION_ERASE == setup_action)
    {
      int32_t ret;
      ret = setup_flash_erase(setup_list[num_appli].code_appli, setup_list[num_appli].version_appli);
      if (ret == 0)
      {
        PrintSetup("FEEPROM Config Erased\r\n");
      }
      else
      {
        PrintSetup("FEEPROM Config NOT Erased\r\n");
      }
    }


    if ((setup_action == SETUP_ACTION_FEEPROM) || (setup_action == SETUP_ACTION_UART) || (setup_action == SETUP_ACTION_DUMP) || (setup_action == SETUP_ACTION_UART))
    {
      /* Select the active configuration and set it */
      if ((setup_action == SETUP_ACTION_FEEPROM) || (setup_action == SETUP_ACTION_UART) || (setup_action == SETUP_ACTION_DUMP))
      {
        /* flash configuration preempts default configuration if exists*/
        setup_config_size = setup_feeprom_config_set(setup_list[num_appli].code_appli, setup_list[num_appli].version_appli, &setup_config_addr);
        if (setup_config_size != 0U)
        {
          /* flash configuration exists => flash configuration set as active */
          setup_source = MENU_SETUP_SOURCE_FEEPROM;
        }
      }

      if (setup_config_size == 0U)
      {
        /* No configuration in flash => default configuration set as active */
        setup_config_size = setup_default_config_set(setup_list[num_appli].default_config, setup_list[num_appli].default_config_size);
        setup_config_addr = setup_default_config;
        setup_source      = MENU_SETUP_SOURCE_DEFAULT;
      }

      if (setup_action == SETUP_ACTION_UART)
      {
        setup_source = MENU_SETUP_SOURCE_UART;
      }

      PrintSetup("\n\r--------------------------------\n\r");
      PrintSetup(" %s Config from %s \n\r", setup_list[num_appli].label_appli, setup_source_string[setup_source]);
      PrintSetup("--------------------------------\n\r");

      menu_utils_set_source(setup_source, setup_config_addr, setup_config_size);


      /* Active configuration selected : set it */
      bad_version = 0U;
      if ((setup_list[num_appli].setup_fnct != NULL) && (setup_config_size != 0U))
      {
        if (setup_action == SETUP_ACTION_UART)
        {
          int32_t string_version_int;
          PrintSetup("\n\rVersion (%d): ", setup_list[num_appli].version_appli);
          size = menu_utils_get_line(string_version, sizeof(string_version));
          PrintSetup("\n\r");
          string_version_int = atoi((char *)string_version);
          if ((size == 0U) || (string_version_int != (int32_t)setup_list[num_appli].version_appli))
          {
            PrintSetup("\n\rBad Appli version \"%s\" : expected \"%d\"\n\r", string_version, setup_list[num_appli].version_appli);
            bad_version = 1U;
          }
        }

        if (bad_version == 0U)
        {
          /* Call setup callback function to set configuration */
          setup_list[num_appli].setup_fnct();
          setup_list[num_appli].config_done = 1U;

#if (FEEPROM_UTILS_FLASH_USED == 1)
          if (setup_action == SETUP_ACTION_UART)
          {
            /* set configuration to update from uart console */
            menu_utils_get_new_config(&setup_config_addr, &setup_config_size);

            /* save updated configuration in feeprom */
            feeprom_utils_save_config_flash(setup_list[num_appli].code_appli, setup_list[num_appli].version_appli, setup_config_addr, setup_config_size);
          }
#endif
        }
      }

      if (SETUP_ACTION_DUMP == setup_action)
      {
        /* call  application dump calback */
        if (setup_list[num_appli].dump_fnct != NULL)
        {
          setup_list[num_appli].dump_fnct();
        }
      }
    }
  }
}

/* External functions BEGIN */

/* allows an application to setup its own setup configuration  */
int32_t setup_record(setup_appli_code_t code_appli, setup_appli_version_t version_appli, MU_CHAR_t *label_appli, void (*setup_fnct)(void), void (*dump_fnct)(void), uint8_t **default_config, uint32_t default_config_size)
{
  int32_t ret;
  if (setup_nb >= SETUP_NUMBER_MAX)
  {
    ret = -1;
  }
  else
  {
    setup_list[setup_nb].code_appli            = code_appli;
    setup_list[setup_nb].version_appli         = version_appli;
    setup_list[setup_nb].label_appli           = label_appli;
    setup_list[setup_nb].setup_fnct            = setup_fnct;
    setup_list[setup_nb].dump_fnct             = dump_fnct;
    setup_list[setup_nb].default_config        = default_config;
    setup_list[setup_nb].default_config_size   = default_config_size;
    setup_list[setup_nb].config_done           = 0U;

    setup_nb++;
    ret = 0;
  }
  return ret;
}

/* setup component init */
uint8_t *setup_get_label_appli(setup_appli_code_t code_appli)
{
  uint8_t *label;
  uint16_t i;
  label = 0;

  for (i = 0U; i < setup_nb; i++)
  {
    if (setup_list[i].code_appli == code_appli)
    {
      label = setup_list[i].label_appli;
    }
  }
  return label;
}

/* setup component init */
void setup_init(void)
{
  app_select_record((AS_CHART_t *)SETUP_APP_SELECT_LABEL, APP_SELECT_SETUP_CODE);
}

/* setup component start */
void setup_start(void)
{
  uint32_t i;
  uint8_t  car;
  uint32_t num_appli;
  int32_t  ret = 0;

  while (1)
  {
    PrintSetup("\n\r--------------------------------\n\r");
    timedate_setup_dump();
    PrintSetup("--------------------------------\n\r");
    PrintSetup("   %s   \n\r", SETUP_APP_SELECT_LABEL);
    PrintSetup("--------------------------------\n\r");
    PrintSetup("Select the component to config:\n\r\n\r");
    PrintSetup("0: Quit\n\r");
    PrintSetup("1: Date/Time (RTC)\n\r");

    for (i = 0U; i < setup_nb; i++)
    {
      PrintSetup("%ld: %s\n\r", i + 2U, setup_list[i].label_appli);
    }
    PrintSetup("8: Get list of config sources\n\r");

#if (FEEPROM_UTILS_FLASH_USED == 1)
    PrintSetup("9: Erase all feeprom configs\n\r");
#endif
    ret = menu_utils_get_uart_char(&car);
    if (ret)
    {
      num_appli = (uint32_t)car - 0x30U;
      if (num_appli == 0U)
      {
        break;
      }

#if (FEEPROM_UTILS_FLASH_USED == 1)
      else if (num_appli == 9U)
      {
        feeprom_utils_flash_erase_all();
        PrintSetup("\n\r-> All feeprom config erased\n\r\n\r");
      }
#endif
      else if (num_appli == 8U)
      {
        setup_get_source();
      }
      else if (num_appli == 1U)
      {
        setup_timedate_handle(num_appli);
      }
      else
      {
        num_appli -= 2U;
        if (num_appli < setup_nb)
        {
          setup_handle(num_appli);
        }
      }
    }
  }
}

/* apply all active setup configurations */
void setup_apply(void)
{
  uint32_t i;
  uint32_t setup_config_size;
  uint8_t *setup_config_addr;
  menu_setup_source_t setup_source;

  for (i = 0U; i < setup_nb; i++)
  {
    if (setup_list[i].config_done == 0U)
    {
      setup_config_size = setup_feeprom_config_set(setup_list[i].code_appli, setup_list[i].version_appli, &setup_config_addr);
      if (setup_config_size == 0U)
      {
        setup_config_size = setup_default_config_set(setup_list[i].default_config, setup_list[i].default_config_size);
        setup_source        = MENU_SETUP_SOURCE_DEFAULT;
        setup_config_addr   = setup_default_config;
      }
      else
      {
        setup_source      = MENU_SETUP_SOURCE_FEEPROM;
      }

      menu_utils_set_source(setup_source, setup_config_addr, setup_config_size);

      PrintSetup("\n\r--------------------------------\n\r");
      PrintSetup(" %s Config from %s \n\r", setup_list[i].label_appli, setup_source_string[setup_source]);
      PrintSetup("--------------------------------\n\r");
      setup_list[i].setup_fnct();
      setup_list[i].config_done = 1U;
    }
  }
}

/* External functions BEGIN */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
