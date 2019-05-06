/**
  ******************************************************************************
  * @file    at_custom_common.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_common.c module
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
#ifndef AT_CUSTOM_COMMON_H
#define AT_CUSTOM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "sysctrl.h"
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
#define _AT_INVALID ((uint32_t) 0xFFFFFFFFU)

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  INTERMEDIATE_CMD = 0,
  FINAL_CMD        = 1,
} atcustom_FinalCmd_t;

typedef void (*ATC_initTypeDef)(atparser_context_t *p_atp_ctxt);
typedef uint8_t (*ATC_checkEndOfMsgCallbackTypeDef)(uint8_t rxChar);
typedef at_status_t (*ATC_getCmdTypeDef)(atparser_context_t *p_atp_ctxt,
                                         uint32_t *p_ATcmdTimeout);
typedef at_endmsg_t (*ATC_extractElementTypeDef)(atparser_context_t *p_atp_ctxt,
                                                 IPC_RxMessage_t *p_msg_in,
                                                 at_element_info_t *element_infos);
typedef at_action_rsp_t (*ATC_analyzeCmdTypeDef)(at_context_t *p_at_ctxt,
                                                 IPC_RxMessage_t *p_msg_in,
                                                 at_element_info_t *element_infos);
typedef at_action_rsp_t (*ATC_analyzeParamTypeDef)(at_context_t *p_at_ctxt,
                                                   IPC_RxMessage_t *p_msg_in,
                                                   at_element_info_t *element_infos);
typedef at_action_rsp_t (*ATC_terminateCmdTypedef)(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);
typedef at_status_t (*ATC_get_rsp)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
typedef at_status_t (*ATC_get_urc)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
typedef at_status_t (*ATC_get_error)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);

typedef struct
{
  uint8_t                            initialized;
  ATC_initTypeDef                    f_init;
  ATC_checkEndOfMsgCallbackTypeDef   f_checkEndOfMsgCallback;
  ATC_getCmdTypeDef                  f_getCmd;
  ATC_extractElementTypeDef          f_extractElement;
  ATC_analyzeCmdTypeDef              f_analyzeCmd;
  ATC_analyzeParamTypeDef            f_analyzeParam;
  ATC_terminateCmdTypedef            f_terminateCmd;
  ATC_get_rsp                        f_get_rsp;
  ATC_get_urc                        f_get_urc;
  ATC_get_error                      f_get_error;
} atcustom_funcPtrs_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
at_status_t atcc_initParsers(sysctrl_device_type_t device_type);
void atcc_init(at_context_t *p_at_ctxt);
ATC_checkEndOfMsgCallbackTypeDef atcc_checkEndOfMsgCallback(at_context_t *p_at_ctxt);
at_status_t atcc_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t atcc_extractElement(at_context_t *p_at_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos);
at_action_rsp_t atcc_analyzeCmd(at_context_t *p_at_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos);
at_action_rsp_t atcc_analyzeParam(at_context_t *p_at_ctxt,
                                  IPC_RxMessage_t *p_msg_in,
                                  at_element_info_t *element_infos);
at_action_rsp_t atcc_terminateCmd(at_context_t *p_at_ctxt, at_element_info_t *element_infos);
at_status_t atcc_get_rsp(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcc_get_urc(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcc_get_error(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
