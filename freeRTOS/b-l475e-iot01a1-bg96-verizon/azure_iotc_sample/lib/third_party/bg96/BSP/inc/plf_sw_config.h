/**
  ******************************************************************************
  * @file    plf_sw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the software configuration of the platform
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
#ifndef PLF_SW_CONFIG_H
#define PLF_SW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_modem_config.h"
#include "plf_stack_size.h"

/* Exported constants --------------------------------------------------------*/

/* Compilation Flag  BEGIN */
/* Stack size trace analysis */
#define STACK_ANALYSIS_TRACE   (0)
#if (STACK_ANALYSIS_TRACE == 1)
#include "stack_analysis.h"
#endif /* STACK_ANALYSIS_TRACE */

/* Compilation Flag  END */

/* Stack Size BEGIN */
/* Number of threads in the Project */

/* Stack Size END */

/* Stack Priority BEGIN */
#define TCPIP_THREAD_PRIO                  osPriorityBelowNormal
#define PPPOSIF_CLIENT_THREAD_PRIO         osPriorityHigh
#define DC_CTRL_THREAD_PRIO                osPriorityNormal
#define DC_TEST_THREAD_PRIO                osPriorityNormal
#define DC_MEMS_THREAD_PRIO                osPriorityNormal
#define DC_EMUL_THREAD_PRIO                osPriorityNormal
#define ATCORE_THREAD_STACK_PRIO           osPriorityNormal
#define CELLULAR_SERVICE_THREAD_PRIO       osPriorityNormal
#define NIFMAN_THREAD_PRIO                 osPriorityNormal
#define HTTPCLIENT_THREAD_PRIO             osPriorityNormal
#define CTRL_THREAD_PRIO                   osPriorityAboveNormal
#define PINGCLIENT_THREAD_PRIO             osPriorityNormal
/* Stack Priority END */

/* IPC config BEGIN */
#define USER_DEFINED_IPC_MAX_DEVICES   (1)
#define USER_DEFINED_IPC_DEVICE_MODEM  (IPC_DEVICE_0)
/* IPC config END */

#define PPP_NETMASK_HEX        0x00FFFFFF    /* 255.255.255.0 */

/* Polling modem period */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#define CST_MODEM_POLLING_PERIOD          (10000U)  /* Polling period = 10s */
#else
#define CST_MODEM_POLLING_PERIOD          (0U)      /* No polling for modem monitoring */
#endif

/* FLASH config mapping */
#define FEEPROM_UTILS_FLASH_USED      (1)
#define FEEPROM_UTILS_LAST_PAGE_ADDR  (FLASH_LAST_PAGE_ADDR)
#define FEEPROM_UTILS_APPLI_MAX       5


/* ================================================= */
/* BEGIN - Middleware components used (expert mode)  */
/* ================================================= */
#define RTOS_USED         (1) /* DO NOT MODIFY THIS VALUE */
#define USE_DATACACHE     (1) /* DO NOT MODIFY THIS VALUE */

/* ================================================= */
/* END - Middleware components used                  */
/* ================================================= */

/* ============================================= */
/* BEGIN - Button polling                        */
/* ============================================= */
#if (USE_DC_MEMS == 1)
#define BB_LEFT_POLLING  (0) /* 0: not activated, 1: activated */
#endif
#define BB_RIGHT_POLLING (0) /* 0: not activated, 1: activated */
#define BB_UP_POLLING    (0) /* 0: not activated, 1: activated */
#define BB_DOWN_POLLING  (0) /* 0: not activated, 1: activated */

#if ((BB_LEFT_POLLING == 1) || (BB_RIGHT_POLLING == 1) || (BB_UP_POLLING == 1) || (BB_DOWN_POLLING == 1))
#define BB_BUTTON_POLLING (1)
#else
#define BB_BUTTON_POLLING (0)
#endif
/* ============================================= */
/* END - Button polling                          */
/* ============================================= */

/* Trace flags BEGIN */
#define SW_DEBUG_VERSION              (1U)   /* 0 for SW release version (no traces), 1 for SW debug version */

#if (SW_DEBUG_VERSION == 1U)
/* ### SOFTWARE DEBUG VERSION :  traces activated ### */
/* trace channels: ITM - UART */
#define TRACE_IF_TRACES_ITM           (0U) /* trace_interface module send traces to ITM */
#define TRACE_IF_TRACES_UART          (1U) /* trace_interface module send traces to UART */
#define USE_PRINTF                    (0U) /* if set to 1, use printf instead of trace_interface module */

/* trace masks allowed */
/* P0, WARN and ERROR traces only */
#define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_WARN | DBL_LVL_ERR)
/* Full traces */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_P1 | DBL_LVL_P2 | DBL_LVL_WARN | DBL_LVL_ERR) */

/* trace module flags : indicate which modules are generating traces */
#define USE_TRACE_TEST                (0U)
#define USE_TRACE_SYSCTRL             (0U)
#define USE_TRACE_ATCORE              (0U)
#define USE_TRACE_ATCUSTOM_MODEM      (0U)
#define USE_TRACE_ATCUSTOM_COMMON     (0U)
#define USE_TRACE_ATDATAPACK          (0U)
#define USE_TRACE_ATPARSER            (0U)
#define USE_TRACE_CELLULAR_SERVICE    (0U)
#define USE_TRACE_ATCUSTOM_SPECIFIC   (0U)
#define USE_TRACE_COM_SOCKETS         (0U)
#define USE_TRACE_HTTP_CLIENT         (0U)
#define USE_TRACE_PING_CLIENT         (0U)
#define USE_TRACE_PPPOSIF             (0U)
#define USE_TRACE_IPC                 (0U)
#define USE_TRACE_DCLIB               (0U)
#define USE_TRACE_DCMEMS              (0U)
#define USE_TRACE_ERROR_HANDLER       (0U)


/* AWS CONFIFPRINTF debug print module flags use these in AWS FreeRTOS implementation */
#define USE_CONFIGPRINTF_SYSCTRL             (0)
#define USE_CONFIGPRINTF_ATCORE              (0)
#define USE_CONFIGPRINTF_ATCUSTOM_MODEM      (0)
#define USE_CONFIGPRINTF_ATCUSTOM_COMMON     (0)
#define USE_CONFIGPRINTF_ATDATAPACK          (0)
#define USE_CONFIGPRINTF_ATPARSER            (0)
#define USE_CONFIGPRINTF_CELLULAR_SERVICE    (0)
#define USE_CONFIGPRINTF_ATCUSTOM_SPECIFIC   (0)
#define USE_CONFIGPRINTF_COM_SOCKETS_IP      (0)
#define USE_CONFIGPRINTF_IPC                 (0)
#define USE_CONFIGPRINTF_DCLIB               (0)
#define USE_CONFIGPRINTF_DCMEMS              (0)
#define USE_CONFIGPRINTF_ERROR_HANDLER		  (0)
/* AWS CONFIFPRINTF debug print module flags END */


#else
/* ### SOFTWARE RELEASE VERSION : no traces  ### */
/* trace channels: ITM - UART */
#define TRACE_IF_TRACES_ITM           (0U) /* DO NOT MODIFY THIS VALUE */
#define TRACE_IF_TRACES_UART          (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_PRINTF                    (0U) /* DO NOT MODIFY THIS VALUE */

/* trace masks allowed */
/* P0, WARN and ERROR traces only */
#define TRACE_IF_MASK       (uint16_t)(0U) /* DO NOT MODIFY THIS VALUE */

/* trace module flags */
#define USE_TRACE_TEST                (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_SYSCTRL             (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATCORE              (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATCUSTOM_MODEM      (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATCUSTOM_COMMON     (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATDATAPACK          (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATPARSER            (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_CELLULAR_SERVICE    (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ATCUSTOM_SPECIFIC   (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_COM_SOCKETS         (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_HTTP_CLIENT         (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_PING_CLIENT         (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_PPPOSIF             (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_IPC                 (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_DCLIB               (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_DCMEMS              (0U) /* DO NOT MODIFY THIS VALUE */
#define USE_TRACE_ERROR_HANDLER       (0U) /* DO NOT MODIFY THIS VALUE */
#endif /* SW_DEBUG_VERSION*/

/* Trace flags END */


/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_SW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
