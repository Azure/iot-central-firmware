/**
  ******************************************************************************
  * @file    feeprom_utils.c
  * @author  MCD Application Team
  * @brief   feeprom utils for setup management
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
#include "plf_config.h"
#include "feeprom_utils.h"
#include "app_select.h"
#include <string.h>
#include "menu_utils.h"

/* Private defines -----------------------------------------------------------*/
#define FEEPROM_UTILS_MAGIC_FLASH_CONFIG ((uint64_t)0x5555AAAA00000002)

/* Private macros ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* flash config header */
typedef struct
{
  uint64_t config_magic;
  uint16_t appli_code;
  uint16_t appli_version;
  uint32_t config_size;
} setup_config_header_t;


/* Private variables ---------------------------------------------------------*/
static FLASH_EraseInitTypeDef EraseInitStruct;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static int32_t feeprom_utils_flash_write(uint32_t data_addr, uint32_t flash_addr, uint32_t Lenght, uint32_t *byteswritten);
static void feeprom_utils_open_flash(void);
static void feeprom_utils_close_flash(void);



/* Functions Definition ------------------------------------------------------*/

/* open flash before write operation */
static void feeprom_utils_open_flash(void)
{
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
}

/* close flash after write operation */
static void feeprom_utils_close_flash(void)
{
  /* Lock the Flash to disable the flash control register access (recommended
   to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
}

/* Write "Lenght" bytes in feeprom at "flash_addr" */
static int32_t feeprom_utils_flash_write(uint32_t data_addr, uint32_t flash_addr, uint32_t Lenght, uint32_t *byteswritten)
{
  int32_t ret;
  ret = 0;

  *byteswritten = 0U;
  if (Lenght > FLASH_PAGE_SIZE)
  {
    Lenght = FLASH_PAGE_SIZE;
  }

  while (*byteswritten < Lenght)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr, *((uint64_t *)data_addr)) == HAL_OK)
    {
      flash_addr = flash_addr + sizeof(uint64_t);
      data_addr  = data_addr  + sizeof(uint64_t);
      *byteswritten    = *byteswritten + sizeof(uint64_t);
      ret = 0;
    }
    else
    {
      PrintSetup("HAL_FLASH_Program error");
      ret = 1;
    }
  }
  return ret;
}


/* External functions BEGIN */

/*  get the flash bank associated to an application (appli_code) */
int32_t feeprom_utils_find_bank(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint32_t *addr, uint32_t *page_number)
{
  setup_config_header_t *config_header;
  uint32_t  flash_addr;
  uint32_t  i;
  int32_t   ret;
  uint8_t  *label;

  ret  = -1;
  *addr        = 0U;
  *page_number = 0U;

  flash_addr = FEEPROM_UTILS_LAST_PAGE_ADDR;
  for (i = 0U; i < (uint32_t)FEEPROM_UTILS_APPLI_MAX ; i++)
  {
    flash_addr = FEEPROM_UTILS_LAST_PAGE_ADDR - i * FLASH_PAGE_SIZE;
    config_header = (setup_config_header_t *)flash_addr;
    if (config_header->config_magic == FEEPROM_UTILS_MAGIC_FLASH_CONFIG)
    {
      if (config_header->appli_code == appli_code)
      {
        if (config_header->appli_version != appli_version)
        {
          *addr        = flash_addr;
          *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
          label = setup_get_label_appli(appli_code);
          PrintSetup("WARNING : \"%s\"  FEEPROM Config Bad format : config erased\r\n", label);

          /* Invalid version of config for this application => the page is erased and made available */
          if (feeprom_utils_flash_erase(*page_number) == 0)
          {
            ret = 1;
          }
        }
        else
        {
          *addr        = flash_addr;
          *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
          ret = 0;
        }
      }
    }
    else
    {
      if (*addr == 0U)
      {
        *addr        = flash_addr;
        *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
        ret = 1;
      }
    }
  }
  return ret ;
}

/*  erase a flash page  */
int32_t feeprom_utils_flash_erase(uint32_t page_number)
{
  uint32_t PAGEError;
  int32_t ret;

  feeprom_utils_open_flash();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Banks       = FLASH_BANK_2;
  EraseInitStruct.Page        = page_number;
  EraseInitStruct.NbPages     = (uint32_t)1;
  if ((ret = (int32_t)HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError)) != 0)
  {
    PrintSetup("HAL_FLASHEx_Erase error");
  }
  feeprom_utils_close_flash();

  return ret;
}

/*  erase all flash pages of the configuration flash bank */
void feeprom_utils_flash_erase_all(void)
{
  uint32_t i;
  for (i = (uint32_t)0; i < (uint32_t)FEEPROM_UTILS_APPLI_MAX ; i++)
  {
    feeprom_utils_flash_erase((uint32_t)FLASH_LAST_PAGE_NUMBER - i);
  }
}

/*  save the configuration of an application in flash */
int32_t feeprom_utils_save_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint8_t *config_addr, uint32_t config_size)
{
  int32_t  res;
  uint32_t byteswritten;
  uint32_t flash_addr;
  uint32_t page_number;
  setup_config_header_t config_header;

  config_header.config_magic  = FEEPROM_UTILS_MAGIC_FLASH_CONFIG;
  config_header.appli_code    = appli_code;
  config_header.appli_version = appli_version;
  config_header.config_size   = config_size;

  byteswritten = (uint32_t)0;

  res = feeprom_utils_find_bank(appli_code, appli_version,  &flash_addr, &page_number);
  if (res >= 0)
  {
    res = feeprom_utils_flash_erase(page_number);
    if (res == 0)
    {
      feeprom_utils_open_flash();
      res = feeprom_utils_flash_write((uint32_t)&config_header, flash_addr, sizeof(setup_config_header_t), &byteswritten);

      flash_addr += sizeof(setup_config_header_t);
      res += feeprom_utils_flash_write((uint32_t)config_addr, flash_addr, config_size, &byteswritten);
      feeprom_utils_close_flash();
    }

    PrintSetup("New config is written in feeprom (%ld bytes)\n\r", byteswritten);
  }
  return res;
}


/*  get the configuration of an application (appli_code) in flash */
int32_t feeprom_utils_read_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint8_t **config_addr, uint32_t *config_size)
{
  int32_t ret = -1;
  uint32_t   flash_addr;
  uint32_t page_number;

  setup_config_header_t *config_header;
  uint8_t              *flash_addr_ptr;

  ret = feeprom_utils_find_bank(appli_code, appli_version, &flash_addr, &page_number);

  if (ret == 0)
  {
    flash_addr_ptr = (uint8_t *)flash_addr;
    config_header  = (setup_config_header_t *)flash_addr;
    *config_size   = config_header->config_size;
    *config_addr   = flash_addr_ptr + sizeof(setup_config_header_t);
    ret = 0;
  }

  return ret;
}

/* External functions END */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
