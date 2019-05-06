/**
  ******************************************************************************
  * @file    at_custom_common.c
  * @author  MCD Application Team
  * @brief   This file provides common code for devices
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
#include "string.h"
#include "at_custom_common.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"
#include "at_custom_modem_api.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_COMMON == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCustom:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCustom:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCustom API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCustom ERROR:" format "\n\r", ## args)
#else
#define PrintINFO(format, args...)  printf("ATCustom:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("ATCustom:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("ATCustom API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("ATCustom ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_COMMON */

/* Private variables ---------------------------------------------------------*/
/* AT custom functions ptrs table */
static atcustom_funcPtrs_t custom_func[DEVTYPE_INVALID] = { 0 };

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
at_status_t atcc_initParsers(sysctrl_device_type_t device_type)
{
  at_status_t retval = ATSTATUS_ERROR;

  /* check if device is already initialized */
  if (custom_func[device_type].initialized == 0U)
  {
    /* Init  AT functions pointers */
    retval = atcma_init_at_func_ptrs(&custom_func[device_type]);
    /* device is initialized now */
    if (retval == ATSTATUS_OK)
    {
      custom_func[device_type].initialized = 1U;
    }
  }
  else
  {
    PrintErr("Device type %d AT functions already initialized", device_type)
  }

  return (retval);
}

void atcc_init(at_context_t *p_at_ctxt)
{
  (* custom_func[p_at_ctxt->device_type].f_init)(&p_at_ctxt->parser);
}

ATC_checkEndOfMsgCallbackTypeDef atcc_checkEndOfMsgCallback(at_context_t *p_at_ctxt)
{
  /* called under interrupion, do not put trace here */
  return (custom_func[p_at_ctxt->device_type].f_checkEndOfMsgCallback);
}

at_status_t atcc_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  at_status_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_getCmd)(&p_at_ctxt->parser, p_ATcmdTimeout);

  PrintDBG("atcc_getCmd returned status = %d", retval)
  return (retval);
}

at_endmsg_t atcc_extractElement(at_context_t *p_at_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  at_endmsg_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_extractElement)(&p_at_ctxt->parser, p_msg_in, element_infos);

  PrintDBG("start idx=%d  end idx=%d  size=%d rank=%d",
           element_infos->str_start_idx, element_infos->str_end_idx,
           element_infos->str_size, element_infos->param_rank)
  PrintDBG("atcc_extractElement returned endmsg = %d", retval)
  return (retval);
}

at_action_rsp_t atcc_analyzeCmd(at_context_t *p_at_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  at_action_rsp_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_analyzeCmd)(p_at_ctxt, p_msg_in, element_infos);

  PrintDBG("atcc_analyzeCmd returned action = 0x%x", retval)
  return (retval);
}

at_action_rsp_t atcc_analyzeParam(at_context_t *p_at_ctxt,
                                  IPC_RxMessage_t *p_msg_in,
                                  at_element_info_t *element_infos)
{
  at_action_rsp_t retval_final;
  /* extract first element to analyze */
  atcc_extractElement(p_at_ctxt, p_msg_in, element_infos);
  if (element_infos->str_size == 0U)
  {
    retval_final = ATACTION_RSP_NO_ACTION;
  }
  else
  {
    retval_final = (*custom_func[p_at_ctxt->device_type].f_analyzeParam)(p_at_ctxt, p_msg_in, element_infos);
  }

  PrintDBG("atcc_analyzeParam returned action = 0x%x", retval_final)
  return (retval_final);
}

at_action_rsp_t atcc_terminateCmd(at_context_t *p_at_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_terminateCmd)(&p_at_ctxt->parser, element_infos);

  PrintDBG("atcc_terminateCmd returned action = 0x%x", retval)
  return (retval);
}

at_status_t atcc_get_rsp(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_get_rsp)(&p_at_ctxt->parser, p_rsp_buf);

  PrintDBG("atcc_get_rsp returned status = %d", retval)
  return (retval);
}

at_status_t atcc_get_urc(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_get_urc)(&p_at_ctxt->parser, p_rsp_buf);

  PrintDBG("atcc_get_urc returned status = %d", retval)
  return (retval);
}

at_status_t atcc_get_error(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*custom_func[p_at_ctxt->device_type].f_get_error)(&p_at_ctxt->parser, p_rsp_buf);

  PrintDBG("atcc_get_error returned status = %d", retval)
  return (retval);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
