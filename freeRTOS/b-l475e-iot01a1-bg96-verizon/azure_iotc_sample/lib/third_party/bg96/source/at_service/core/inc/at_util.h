/**
  ******************************************************************************
  * @file    at_util.h
  * @author  MCD Application Team
  * @brief   Header for at_util.c module
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
#ifndef AT_UTIL_H
#define AT_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
uint32_t ATutil_ipow(uint32_t base, uint16_t exp);
uint32_t ATutil_convertStringToInt(uint8_t *p_string, uint16_t size);
uint64_t ATutil_convertStringToInt64(uint8_t *p_string, uint16_t size);
uint64_t ATutil_convertHexaStringToInt64(uint8_t *p_string, uint16_t size);
void     ATutil_convertStringToUpperCase(uint8_t *p_string, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* AT_UTIL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
