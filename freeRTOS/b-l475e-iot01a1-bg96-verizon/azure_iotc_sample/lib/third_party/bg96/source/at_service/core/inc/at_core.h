/**
  ******************************************************************************
  * @file    at_core.h
  * @author  MCD Application Team
  * @brief   Header for at_core.c module
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
#ifndef AT_CORE_H_
#define AT_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sysctrl.h"
#include "plf_config.h"

#if (RTOS_USED == 1)
#include "cmsis_os.h"
#endif /* RTOS_USED */

/* Exported constants --------------------------------------------------------*/

/* Define buffers max sizes */
#define ATCMD_MAX_NAME_SIZE  ((uint16_t) 32U)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#define ATCMD_MAX_CMD_SIZE   ((uint16_t) 1600U)
#else
#define ATCMD_MAX_CMD_SIZE   ((uint16_t) 128U)
#endif /* USE_SOCKETS_TYPE */

#define ATCMD_MAX_BUF_SIZE   ((uint16_t) 128U) /* this is the maximum size allowed for p_cmd_in_buf and p_rsp_buf
                                               * which are buffers shared between ATCore and upper layer.
                                               * Size is limited because for big structures (or data buffers), only
                                               * a ptr should be send using DATAPACK writes functions.
                                               * 128 is the maximum size allowed to send data without ptrs (ie using
                                               * DATAPACK_writeStruct()).
                                               * Value of 128 has been choosen to add a marge for the maximum value seen
                                               * until now (71).
                                               * If struct size exceed this value, an error is raised.
                                               */

#define ATCMD_MAX_DELAY      ((uint32_t) 0xFFFFFFFFU) /* No timeout in this case */

#define SID_INVALID               (0U)
#define CELLULAR_SERVICE_START_ID (100U)
#define WIFI_SERVICE_START_ID     (300U)

/* at_action_send_t
 * code returned when preparing a command to send
 */
#define ATACTION_SEND_NO_ACTION          ((uint16_t) 0x0000) /* No action defined yet (used to reset flags) */
#define ATACTION_SEND_WAIT_MANDATORY_RSP ((uint16_t) 0x0001) /* Wait a response (mandatory: if not received, this is an error) */
#define ATACTION_SEND_TEMPO              ((uint16_t) 0x0002) /* Temporisation for waiting an eventual response or event */
#define ATACTION_SEND_ERROR              ((uint16_t) 0x0004) /* Error: unknown msg, etc... */
#define ATACTION_SEND_FLAG_LAST_CMD      ((uint16_t) 0x8000)  /* when this flag is set, this is the final command */

/* at_action_rsp_t
 * code returned when analyzing a response
 */
#define ATACTION_RSP_NO_ACTION          ((uint16_t) 0x0000) /* No action defined yet (used to reset flags) */
#define ATACTION_RSP_FRC_END            ((uint16_t) 0x0001) /* Received Final Result Code (OK, CONNECT...), no more AT command to send */
#define ATACTION_RSP_FRC_CONTINUE       ((uint16_t) 0x0002) /* Received Final Result Code (OK, CONNECT...), still AT command to send */
#define ATACTION_RSP_ERROR              ((uint16_t) 0x0004) /* Error: unknown msg, etc... */
#define ATACTION_RSP_INTERMEDIATE       ((uint16_t) 0x0008) /* FRC not received for this AT transaction */
#define ATACTION_RSP_IGNORED            ((uint16_t) 0x0010) /* Message ignored */
#define ATACTION_RSP_URC_IGNORED        ((uint16_t) 0x0020) /* Result Code received, to ignore (default) */
#define ATACTION_RSP_URC_FORWARDED      ((uint16_t) 0x0040) /* Unsolicited Result Code received, to forward to client */
#define ATACTION_RSP_FLAG_DATA_MODE     ((uint16_t) 0x8000) /* when this flag is set, DATA mode activated in modem */

/* Exported types ------------------------------------------------------------*/
typedef uint8_t AT_CHAR_t;

typedef enum
{
  AT_FALSE = 0,
  AT_TRUE  = 1,
} at_bool_t;

typedef enum
{
  ATSTATUS_OK = 0,
  ATSTATUS_ERROR,
  ATSTATUS_TIMEOUT,
  ATSTATUS_OK_PENDING_URC,
} at_status_t;

typedef enum
{
  ATENDMSG_YES = 0,
  ATENDMSG_NO,
  ATENDMSG_ERROR,
} at_endmsg_t;

typedef enum
{
  ATTYPE_UNKNOWN_CMD = 0, /* unknown AT cmd type                 */
  ATTYPE_TEST_CMD,        /* AT test cmd type:      AT+<x>=?     */
  ATTYPE_READ_CMD,        /* AT read cmd type:      AT+<x>?      */
  ATTYPE_WRITE_CMD,       /* AT write cmd type:     AT+<x>=<...> */
  ATTYPE_EXECUTION_CMD,   /* AT execution cmd type: AT+<x>       */
  ATTYPE_NO_CMD,          /* No command to send */
  ATTYPE_RAW_CMD,         /* RAW command to send (Non AT cmd type) */
} at_type_t;

typedef unsigned int at_action_send_t; /* uses ATACTION_SEND_xxx constants defined above */

typedef unsigned int at_action_rsp_t; /* uses ATACTION_RSP_xxx constants defined above */

typedef struct
{
  at_type_t    type;
  uint32_t     id;
  uint8_t      name[ATCMD_MAX_NAME_SIZE];
  uint8_t      params[ATCMD_MAX_CMD_SIZE];
  uint32_t     raw_cmd_size;                   /* raw_cmd_size is used only for raw commands */
} atcmd_desc_t;

typedef uint16_t at_msg_t;
typedef uint16_t at_handle_t;
typedef uint8_t  at_buf_t;
typedef void (* event_callback_t)(void);
typedef void (* urc_callback_t)(at_buf_t *p_rsp_buf);

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
at_status_t  AT_init(void);
at_status_t  AT_deinit(void);
at_handle_t  AT_open(sysctrl_info_t *p_device_infos, event_callback_t event_callback, urc_callback_t urc_callback);
at_status_t  AT_close(at_handle_t athandle);
at_status_t  AT_reset_context(at_handle_t athandle);
at_status_t  AT_sendcmd(at_handle_t athandle, at_msg_t msg_in_id, at_buf_t *p_cmd_in_buf, at_buf_t *p_rsp_buf);

#if (RTOS_USED == 1)
at_status_t atcore_task_start(osPriority taskPrio, uint32_t stackSize);
#else
at_status_t  AT_getevent(at_handle_t athandle, at_buf_t *p_rsp_buf);
#endif /* RTOS_USED */

#ifdef __cplusplus
}
#endif

#endif /* AT_CORE_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

