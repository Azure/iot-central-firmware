/**
  ******************************************************************************
  * @file    cellular_service_task.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_task.c module
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
#ifndef CELLULAR_SERVICE_TASK_H
#define CELLULAR_SERVICE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "cellular_service.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  CST_MODEM_INIT_STATE,
  CST_MODEM_RESET_STATE,
  CST_MODEM_ON_STATE,
  CST_MODEM_ON_ONLY_STATE,
  CST_MODEM_POWERED_ON_STATE,
  CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE,
  CST_WAITING_FOR_NETWORK_STATUS_STATE,
  CST_NETWORK_STATUS_OK_STATE,
  CST_MODEM_REGISTERED_STATE,
  CST_MODEM_PDN_ACTIVATE_STATE,
  CST_MODEM_PDN_ACTIVATED_STATE,
  CST_MODEM_DATA_READY_STATE,
  CST_MODEM_REPROG_STATE,
  CST_MODEM_FAIL_STATE,
  CST_MODEM_NETWORK_STATUS_FAIL_STATE,
} CST_state_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern osThreadId CST_cellularServiceThreadId;

extern CST_state_t CST_get_state(void);

extern CS_Status_t CST_radio_on(void);

extern CS_Status_t CST_modem_power_on(void);

extern CS_Status_t CST_cellular_service_init(void);

extern CS_Status_t CST_cellular_service_start(void);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_TASK_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

