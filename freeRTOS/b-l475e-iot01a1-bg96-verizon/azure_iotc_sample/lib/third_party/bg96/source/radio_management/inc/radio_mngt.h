/**
  ******************************************************************************
  * @file    radio_mngt.h
  * @author  MCD Application Team
  * @brief   Header for radio_mngt.c module
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
#ifndef RADIO_MNGT_H
#define RADIO_MNGT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "error_handler.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  RADIO_MNGT_OK = 0x00,
  RADIO_MNGT_KO
} radio_mngt_status_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  radio on
  * @retval radio_mngt_status_t   return status
  */
extern radio_mngt_status_t radio_mngt_radio_on(void);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_MNGT_H */

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/

