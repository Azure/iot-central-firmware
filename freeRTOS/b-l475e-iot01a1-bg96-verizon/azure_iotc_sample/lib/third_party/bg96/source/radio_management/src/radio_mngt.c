/**
  ******************************************************************************
  * @file    radio_mngt.c
  * @author  MCD Application Team
  * @brief   This file contains radio management
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
#include "error_handler.h"
#include "radio_mngt.h"
#include "cellular_service_task.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  radio on
  * @retval radio_mngt_status_t   return status
  */
radio_mngt_status_t radio_mngt_radio_on(void)
{
  return (radio_mngt_status_t)CST_radio_on();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

