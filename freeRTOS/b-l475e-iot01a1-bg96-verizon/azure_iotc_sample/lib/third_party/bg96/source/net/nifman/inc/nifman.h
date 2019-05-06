/**
  ******************************************************************************
  * @file    nifman.h
  * @author  MCD Application Team
  * @brief   Header for nifman.c module
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
#ifndef NIFMAN_H
#define NIFMAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  NIFMAN_OK = 0x00,
  NIFMAN_ERROR
} nifman_status_t;

/* External variables --------------------------------------------------------*/
extern osThreadId  nifman_ThreadId;

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  component init
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_init(void);

/**
  * @brief  component start
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_start(void);

#ifdef __cplusplus
}
#endif

#endif /* NIFMAN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
