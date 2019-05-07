/**
  ******************************************************************************
  * @file    feeprom_utils.h
  * @author  MCD Application Team
  * @brief   Header for feeprom_utils.c module
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
#ifndef FEEPROM_UTILS_H
#define FEEPROM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "setup.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/*  get the flash bank associated to an application (appli_code) */
int32_t feeprom_utils_find_bank(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint32_t *addr, uint32_t *page_number);

/*  erase page of flash  */
int32_t feeprom_utils_flash_erase(uint32_t page_number);

/*  erase all flash pages of the configuration flash bank */
void    feeprom_utils_flash_erase_all(void);

/*  save the configuration of an application in flash */
int32_t feeprom_utils_save_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint8_t *config_addr, uint32_t config_size);

/*  get the configuration of an application (appli_code) in flash */
int32_t feeprom_utils_read_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version, uint8_t **config_addr, uint32_t *config_size);


#ifdef __cplusplus
}
#endif

#endif /* FEEPROM_UTILS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


