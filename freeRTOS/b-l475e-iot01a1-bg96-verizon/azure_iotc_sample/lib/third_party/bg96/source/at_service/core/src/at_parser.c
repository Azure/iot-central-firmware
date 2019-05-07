/**
  ******************************************************************************
  * @file    at_parser.c
  * @author  MCD Application Team
  * @brief   This file provides code for AT Parser
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
#include "at_parser.h"
#include "at_custom_common.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define FILTER_SOCKET_TRACES   (1U)   /* filter display of big frame in trace output */
#define FILTER_DEFAULT_LENGH   (60U)  /* default lengh to activate the filter */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATPARSER == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATParser:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATParser:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATParser API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATParser ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P0, (char *)pbuf, size);
#define PrintBufHexa(pbuf, size)   TracePrintBufHex(DBG_CHAN_ATCMD, DBL_LVL_P0, (char *)pbuf, size);
#define PrintINDENT()              TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "    ")
#else
#define PrintINFO(format, args...)  printf("ATParser:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("ATParser:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("ATParser API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("ATParser ERROR:" format "\n\r", ## args);
#define PrintBuf(pbuf, size)        do {} while(0);
#define PrintBufHexa(pbuf, size)    do {} while(0);
#define PrintINDENT()               printf("    ");
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(pbuf, size)        do {} while(0);
#define PrintBufHexa(pbuf, size)    do {} while(0);
#define PrintINDENT()               do {} while(0);
#endif /* USE_TRACE_ATPARSER */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void reset_parser_context(atparser_context_t *p_atp_ctxt);
static void reset_current_command(atparser_context_t *p_atp_ctxt);
static void display_buffer(at_context_t *p_at_ctxt, uint8_t *p_buf, uint16_t buf_size, uint8_t is_TX_buf);

/* Functions Definition ------------------------------------------------------*/
at_status_t ATParser_initParsers(sysctrl_device_type_t device_type)
{
  return (atcc_initParsers(device_type));
}

at_status_t ATParser_init(at_context_t *p_at_ctxt, IPC_CheckEndOfMsgCallbackTypeDef *p_checkEndOfMsgCallback)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATParser_init()")

  /* reset request context */
  reset_parser_context(&p_at_ctxt->parser);

  /* get callback pointer */
  *p_checkEndOfMsgCallback = atcc_checkEndOfMsgCallback(p_at_ctxt);

  /* default termination string for AT command: <CR>
   * this value can be changed in ATCustom init if needed
   */
  sprintf((char *)p_at_ctxt->parser.endstr, "\r");

  /* call custom init */
  atcc_init(p_at_ctxt);

  return (retval);
}

at_status_t  ATParser_process_request(at_context_t *p_at_ctxt,
                                      at_msg_t msg_id, at_buf_t *p_cmd_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATParser_process_request()")

  /* reset the context */
  reset_parser_context(&p_at_ctxt->parser);

  /* save ptr on input cmd buffer */
  p_at_ctxt->parser.p_cmd_input = p_cmd_buf;

  /* set the SID */
  p_at_ctxt->parser.current_SID = msg_id;

  return (retval);
}

at_action_send_t  ATParser_get_ATcmd(at_context_t *p_at_ctxt,
                                     uint8_t *p_ATcmdBuf, uint16_t *p_ATcmdSize, uint32_t *p_ATcmdTimeout)
{
  at_action_send_t action = ATACTION_SEND_NO_ACTION;
  PrintAPI("enter ATParser_get_ATcmd()")

  /* init command size to 0 */
  *p_ATcmdSize = 0U;
  reset_current_command(&p_at_ctxt->parser);

  /* get the next command to send and set timeout value */
  if (atcc_getCmd(p_at_ctxt, p_ATcmdTimeout) != ATSTATUS_OK)
  {
    PrintDBG("parser f_getCmd error")
    action = ATACTION_SEND_ERROR;
  }

  if (action != ATACTION_SEND_ERROR)
  {
    /* Build the command string */
    if (ATTYPE_WRITE_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      /* get additional parameters for the command if any */
      sprintf((char *)p_ATcmdBuf, "AT%s=%s%s",
              p_at_ctxt->parser.current_atcmd.name, p_at_ctxt->parser.current_atcmd.params, p_at_ctxt->parser.endstr);
      *p_ATcmdSize = (uint16_t)strlen((char *)p_ATcmdBuf);
    }
    else if (ATTYPE_EXECUTION_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      /* get additional parameters for the command if any */
      sprintf((char *)p_ATcmdBuf, "AT%s%s%s",
              p_at_ctxt->parser.current_atcmd.name, p_at_ctxt->parser.current_atcmd.params, p_at_ctxt->parser.endstr);
      *p_ATcmdSize = (uint16_t)strlen((char *)p_ATcmdBuf);
    }
    else if (ATTYPE_READ_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      sprintf((char *)p_ATcmdBuf, "AT%s?%s", p_at_ctxt->parser.current_atcmd.name, p_at_ctxt->parser.endstr);
      *p_ATcmdSize = (uint16_t)strlen((char *)p_ATcmdBuf);
    }
    else if (ATTYPE_TEST_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      sprintf((char *)p_ATcmdBuf, "AT%s=?%s", p_at_ctxt->parser.current_atcmd.name, p_at_ctxt->parser.endstr);
      *p_ATcmdSize = (uint16_t)strlen((char *)p_ATcmdBuf);
    }
    else if (ATTYPE_RAW_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      /* RAW command: command with NON-AT format
      * send it as provided without header and without end string
      * raw cmd content has been copied into parser.current_atcmd.params
      * its size is in parser.current_atcmd.raw_cmd_size
      *
      */
      if ((p_at_ctxt->parser.current_atcmd.raw_cmd_size != 0U)
          && (p_at_ctxt->parser.current_atcmd.raw_cmd_size <= ATCMD_MAX_CMD_SIZE))
      {
        memcpy((void *)p_ATcmdBuf, (void *)p_at_ctxt->parser.current_atcmd.params, p_at_ctxt->parser.current_atcmd.raw_cmd_size);
        *p_ATcmdSize = (uint16_t)p_at_ctxt->parser.current_atcmd.raw_cmd_size;
      }
      else
      {
        PrintErr("Error with RAW command size = %ld", p_at_ctxt->parser.current_atcmd.raw_cmd_size)
      }
    }
    else if (ATTYPE_NO_CMD == p_at_ctxt->parser.current_atcmd.type)
    {
      /* no command to send */
      PrintDBG("no command to send")
      *p_ATcmdSize = 0U;
    }
    else
    {
      PrintErr("Error, invalid command type")
      action = ATACTION_SEND_ERROR;
    }
  }

  if (action != ATACTION_SEND_ERROR)
  {
    /* test if cmd is invalid */
    if (p_at_ctxt->parser.current_atcmd.id == _AT_INVALID)
    {
      *p_ATcmdSize = 0U;
    }

    /* Prepare returned code (if no error) */
    if (p_at_ctxt->parser.answer_expected == CMD_MANDATORY_ANSWER_EXPECTED)
    {
      action |= ATACTION_SEND_WAIT_MANDATORY_RSP;
    }
    else if (p_at_ctxt->parser.answer_expected == CMD_OPTIONAL_ANSWER_EXPECTED)
    {
      action |= ATACTION_SEND_TEMPO;
    }
    else /* NO ANSWER EXPECTED */
    {
      PrintErr("Invalid answer_expected value")
      action = ATACTION_SEND_ERROR;
    }
  }

  if (action != ATACTION_SEND_ERROR)
  {
    /* set last command flag if needed */
    if (p_at_ctxt->parser.is_final_cmd == 1U)
    {
      action |= ATACTION_SEND_FLAG_LAST_CMD;
    }

    /* DUMP SEND BUFFER */
    display_buffer(p_at_ctxt, (uint8_t *)p_ATcmdBuf, (uint16_t)*p_ATcmdSize, 1U);
  }

  PrintDBG("ATParser_get_ATcmd returned action = 0x%x", action)
  return (action);
}

at_action_rsp_t ATParser_parse_rsp(at_context_t *p_at_ctxt, IPC_RxMessage_t *p_message)
{
  at_action_rsp_t cmd_retval = ATACTION_RSP_ERROR, param_retval = ATACTION_RSP_IGNORED, final_retval, clean_retval;
  at_endmsg_t msg_end = ATENDMSG_NO;
  at_element_info_t element_infos = { .current_parse_idx = 0, .cmd_id_received = _AT_INVALID, .param_rank = 0U,
                                      .str_start_idx = 0, .str_end_idx = 0, .str_size = 0
                                    };
  uint16_t data_mode;

  PrintAPI("enter ATParser_parse_rsp()")

  /* DUMP RECEIVE BUFFER */
  display_buffer(p_at_ctxt, (uint8_t *)&p_message->buffer[0], (uint16_t)p_message->size, 0U);

  /* extract next element to analyze */
  msg_end = atcc_extractElement(p_at_ctxt, p_message, &element_infos);

  /* Search for command name */
  cmd_retval = atcc_analyzeCmd(p_at_ctxt, p_message, &element_infos);

  /* extract return code only (remove data mode flag) */
  clean_retval = (at_action_rsp_t)(cmd_retval & ~(at_action_rsp_t)ATACTION_RSP_FLAG_DATA_MODE);
  data_mode = (((uint16_t)cmd_retval & (uint16_t)ATACTION_RSP_FLAG_DATA_MODE) != 0U) ? (uint16_t)1U : (uint16_t)0U;
  PrintDBG("analyzeCmd retval = %d (DATA mode=%d) msg_end = %d", clean_retval, data_mode, (msg_end == ATENDMSG_YES))

  if ((msg_end != ATENDMSG_YES) && (cmd_retval != ATACTION_RSP_ERROR))
  {
    PrintDBG("proceed to params")

    /* Search for command parameters */
    param_retval = atcc_analyzeParam(p_at_ctxt, p_message, &element_infos);
    if (param_retval != ATACTION_RSP_IGNORED)
    {
      /* action has been modified by analysis of parameters */
      PrintDBG("action modified by analysis of params: %d to %d", cmd_retval, param_retval)
      clean_retval = param_retval;
    }
  }

  /* if AT cmd treatment is finished, check if another AT cmd should be sent after */
  if (clean_retval == ATACTION_RSP_FRC_END)
  {
    /* final result code for current AT cmd: clean related context params */
    final_retval = atcc_terminateCmd(p_at_ctxt, &element_infos);
    if (final_retval == ATACTION_RSP_ERROR)
    {
      clean_retval = ATACTION_RSP_ERROR;
    }
    /* do we have another command to send for this SID ? */
    else if (p_at_ctxt->parser.is_final_cmd == 0U)
    {
      clean_retval = ATACTION_RSP_FRC_CONTINUE;
    }
    else
    {
      /* ignore */
    }

    /* current CMD treament is finished: reset command context */
    reset_current_command(&p_at_ctxt->parser);
  }

  /* reintegrate data mode flag if needed */
  if (data_mode)
  {
    cmd_retval = (at_action_rsp_t)(clean_retval | ATACTION_RSP_FLAG_DATA_MODE);
  }
  else
  {
    cmd_retval = clean_retval;
  }

  PrintDBG("ATParser_parse_rsp returned action = 0x%x", cmd_retval)
  return (cmd_retval);
}

at_status_t ATParser_get_rsp(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATParser_get_rsp()")

  retval = atcc_get_rsp(p_at_ctxt, p_rsp_buf);

  /* current SID treament is finished, reset parser context */
  reset_parser_context(&p_at_ctxt->parser);

  return (retval);
}

at_status_t ATParser_get_urc(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATParser_get_urc()")

  retval = atcc_get_urc(p_at_ctxt, p_rsp_buf);
  return (retval);
}

at_status_t ATParser_get_error(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATParser_get_error()")

  retval = atcc_get_error(p_at_ctxt, p_rsp_buf);

  return (retval);
}

void ATParser_abort_request(at_context_t *p_at_ctxt)
{
  PrintAPI("enter ATParser_abort_request()")
  reset_parser_context(&p_at_ctxt->parser);
}

/* Private function Definition -----------------------------------------------*/
static void reset_parser_context(atparser_context_t *p_atp_ctxt)
{
  p_atp_ctxt->current_SID = SID_INVALID;

  p_atp_ctxt->step = 0U;
  p_atp_ctxt->answer_expected = CMD_MANDATORY_ANSWER_EXPECTED;
  p_atp_ctxt->is_final_cmd = 1U;
  p_atp_ctxt->cmd_timeout = 0U;

  reset_current_command(p_atp_ctxt);

  p_atp_ctxt->p_cmd_input = NULL;
}

static void reset_current_command(atparser_context_t *p_atp_ctxt)
{
  p_atp_ctxt->current_atcmd.id = _AT_INVALID;
  p_atp_ctxt->current_atcmd.type = ATTYPE_UNKNOWN_CMD;
  memset((void *)&p_atp_ctxt->current_atcmd.name[0], 0, sizeof(uint8_t) * (ATCMD_MAX_NAME_SIZE));
  memset((void *)&p_atp_ctxt->current_atcmd.params[0], 0, sizeof(uint8_t) * (ATCMD_MAX_NAME_SIZE));
  p_atp_ctxt->current_atcmd.raw_cmd_size = 0U;
}

static void display_buffer(at_context_t *p_at_ctxt, uint8_t *p_buf, uint16_t buf_size, uint8_t is_TX_buf)
{
  uint8_t print_in_hexa = 0U; /* default value : if 1, print in hexa */

  /* print header */
  PrintINDENT()
  if (is_TX_buf == 1U)
  {
    PrintINFO("*** SEND (size=%d) ***", buf_size)
    /* only RAW buffers are printed it in hexa */
    if (p_at_ctxt->parser.current_atcmd.type != ATTYPE_RAW_CMD)
    {
      print_in_hexa = 0U;
    }
  }
  else
  {
    /* Do not print if we received only <CR><LF> */
    if (!(buf_size == 2U) && (p_buf[0] == 0xDU) && (p_buf[1] == 0xAU))
    {
      PrintINFO("*** RECEIVE (size=%d) ***", buf_size)
      if (buf_size >= 2U)
      {
        /* if buffer starts with <CR><LF>, we can print it in char mode */
        if ((p_buf[0] == 0xDU) && (p_buf[1] == 0xAU))
        {
          print_in_hexa = 0U;
        }
      }
    }
  }
  /* PrintINDENT() */

  if (buf_size != 0U)
  {
#if (FILTER_SOCKET_TRACES == 1U)
    if (buf_size > FILTER_DEFAULT_LENGH)
    {
      PrintINFO(" Big frame (display deactivated) ")
    }
    else
    {
      if (print_in_hexa == 1U)
      {
        PrintBufHexa((uint8_t *)p_buf, buf_size)
      }
      else
      {
        PrintBuf((uint8_t *)p_buf, buf_size)
      }
    }
#else
    if (print_in_hexa == 1U)
    {
      PrintBufHexa((const char *)p_buf, buf_size)
    }
    else
    {
      PrintBuf((const char *)p_buf, buf_size)
    }
#endif /* FILTER_SOCKET_TRACES */
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
