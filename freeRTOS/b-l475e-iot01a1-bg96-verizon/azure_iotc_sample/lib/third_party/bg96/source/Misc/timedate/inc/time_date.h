/**
  ******************************************************************************
  * @file    time_date.h
  * @author  MCD Application Team
  * @brief   Header for time_date.c module
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
#ifndef TIMEDATE_H
#define TIMEDATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
int32_t timedate_init(void);
int32_t timedate_set(char *dateStr);
int32_t timedate_http_date_set(char *dateStr);
void timedate_setup_handler(void);
void timedate_setup_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMEDATE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
