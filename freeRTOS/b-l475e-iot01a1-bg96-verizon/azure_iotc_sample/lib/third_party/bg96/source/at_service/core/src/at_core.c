/**
  ******************************************************************************
  * @file    at_core.c
  * @author  MCD Application Team
  * @brief   This file provides code for AT Core
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
#include "ipc_common.h"
#include "at_core.h"
#include "at_parser.h"
#include "error_handler.h"
#include "plf_config.h"
/* following file added to check SID for DATA suspend/resume cases */
#include "cellular_service_int.h"

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCORE == 1U)
#if (USE_PRINTF  == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCore:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCore:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCore API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCore ERROR:" format "\n\r", ## args)
#else
#define PrintINFO(format, args...)  printf("ATCore:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("ATCore:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("ATCore API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("ATCore ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCORE */

#define LogError(ErrId, gravity)   ERROR_Handler(DBG_CHAN_ATCMD, (ErrId), (gravity))

/* Private defines -----------------------------------------------------------*/
#define ATCORE_MAX_HANDLES   ((uint32_t) DEVTYPE_INVALID)

/* specific debug flags */
#define DBG_REQUEST_DURATION (0)  /* trace the time taken for each request and count disable/enable IT */

#if (RTOS_USED == 1)
/* Private defines -----------------------------------------------------------*/
#define ATCORE_SEM_WAIT_ANSWER_COUNT     (1)
#define ATCORE_SEM_SEND_COUNT            (1)
#define MSG_IPC_RECEIVED_SIZE (uint32_t) (16)
#define SIG_IPC_MSG                      (1U) /* signals definition for IPC message queue */

/* Global variables ----------------------------------------------------------*/
/* ATCore task handler */
osThreadId atcoreTaskId = NULL;

/* Private variables ---------------------------------------------------------*/
/* this semaphore is used for waiting for an answer from Modem */
static osSemaphoreId s_WaitAnswer_SemaphoreId = NULL;
/* this semaphore is used to confirm that a msg has been sent (1 semaphore per device) */
#define mySemaphoreDef(name,index)  \
const osSemaphoreDef_t os_semaphore_def_##name##index = { 0 }
#define mySemaphore(name,index)  \
&os_semaphore_def_##name##index
/* Queues definition */
/* this queue is used by IPC to inform that messages are ready to be retrieved */
osMessageQId q_msg_IPC_received_Id;

/* Private function prototypes -----------------------------------------------*/
static at_status_t findMsgReceivedHandle(at_handle_t *athandle);
static void ATCoreTaskBody(void const *argument);
#endif /* RTOS_USED */

#if (DBG_REQUEST_DURATION == 1)
static uint16_t count_disable = 0;
static uint16_t count_enable  = 0;
#define COUNT_MAX_VAL  (0xFFFFFFFF)
#endif /* DBG_REQUEST_DURATION */

/* Private variables ---------------------------------------------------------*/
static uint8_t         AT_Core_initialized = 0U;
static AT_CHAR_t       build_atcmd[ATCMD_MAX_CMD_SIZE] = {0};
static IPC_Handle_t    ipcHandleTab[ATCORE_MAX_HANDLES];
static at_context_t    at_context[ATCORE_MAX_HANDLES];
static urc_callback_t  register_URC_callback[ATCORE_MAX_HANDLES];
static IPC_RxMessage_t  msgFromIPC[ATCORE_MAX_HANDLES];        /* array of IPC msg (1 per ATCore handler) */
static __IO uint8_t     MsgReceived[ATCORE_MAX_HANDLES] = {0}; /* array of rx msg counters (1 per ATCore handler) */

#if (RTOS_USED == 0)
static event_callback_t    register_event_callback[ATCORE_MAX_HANDLES];
#endif

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void msgReceivedCallback(IPC_Handle_t *ipcHandle);
static void msgSentCallback(IPC_Handle_t *ipcHandle);
static uint8_t find_index(IPC_Handle_t *ipcHandle);

static at_status_t process_AT_transaction(at_handle_t athandle, at_msg_t msg_in_id, at_buf_t *p_rsp_buf);
static at_status_t allocate_ATHandle(uint16_t *athandle);
static at_status_t waitOnMsgUntilTimeout(uint16_t athandle, uint32_t Tickstart, uint32_t Timeout);
static at_status_t sendToIPC(at_handle_t athandle,
                             uint8_t *cmdBuf, uint16_t cmdSize);
static at_status_t waitFromIPC(at_handle_t athandle,
                               uint32_t tickstart, uint32_t cmdTimeout, IPC_RxMessage_t *p_msg);
static at_action_rsp_t analyze_action_result(at_handle_t athandle, at_action_rsp_t val);

static void IRQ_DISABLE(void);
static void IRQ_ENABLE(void);

/* Functions Definition ------------------------------------------------------*/
at_status_t  AT_init(void)
{
  uint8_t idx;

  PrintAPI("enter AT_init()")

  /* should be called once */
  if (AT_Core_initialized == 1U)
  {
    LogError(1, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  /* Initialize at_context */
  for (idx = 0U; idx < ATCORE_MAX_HANDLES; idx++)
  {
    MsgReceived[idx] = 0U;
    register_URC_callback[idx] = NULL;
#if (RTOS_USED == 0)
    register_event_callback[idx] = NULL;
#endif
    at_context[idx].device_type = DEVTYPE_INVALID;
    at_context[idx].in_data_mode = AT_FALSE;
    at_context[idx].processing_cmd = 0U;
    at_context[idx].dataSent = AT_FALSE;
#if (RTOS_USED == 1)
    at_context[idx].action_flags = ATACTION_RSP_NO_ACTION;
    at_context[idx].p_rsp_buf = NULL;
    at_context[idx].s_SendConfirm_SemaphoreId = NULL;
#endif

    memset((void *)&at_context[idx].parser, 0, sizeof(atparser_context_t));
  }

  AT_Core_initialized = 1U;
  return (ATSTATUS_OK);
}

at_status_t  AT_deinit(void)
{
  at_status_t retval = ATSTATUS_OK;
  uint8_t idx;

  PrintAPI("enter AT_deinit()")

  /* Reinitialize at_context */
  for (idx = 0U; idx < ATCORE_MAX_HANDLES; idx++)
  {
    MsgReceived[idx] = 0U;
    register_URC_callback[idx] = NULL;
#if (RTOS_USED == 0)
    register_event_callback[idx] = NULL;
#endif
    at_context[idx].device_type = DEVTYPE_INVALID;
    at_context[idx].in_data_mode = AT_FALSE;
    at_context[idx].processing_cmd = 0U;
    at_context[idx].dataSent = AT_FALSE;
#if (RTOS_USED == 1)
    at_context[idx].action_flags = ATACTION_RSP_NO_ACTION;
    at_context[idx].p_rsp_buf = NULL;
    at_context[idx].s_SendConfirm_SemaphoreId = NULL;
#endif

    memset((void *)&at_context[idx].parser, 0, sizeof(atparser_context_t));
  }

  AT_Core_initialized = 0U;

  return (retval);
}

at_handle_t  AT_open(sysctrl_info_t *p_device_infos, event_callback_t event_callback, urc_callback_t urc_callback)
{
#if (RTOS_USED == 1)
  /*UNUSED(event_callback);*/ /* used only for non-RTOS */
#endif /* RTOS_USED */

  uint16_t affectedHandle;
  IPC_CheckEndOfMsgCallbackTypeDef custom_checkEndOfMsgCallback = NULL;

  PrintAPI("enter AT_open()")

  /* Initialize parser for required device type */
  if (ATParser_initParsers(p_device_infos->type) != ATSTATUS_OK)
  {
    LogError(2, ERROR_FATAL);
    return (ATSTATUS_ERROR);
  }

  /* allocate a free handle */
  if (allocate_ATHandle(&affectedHandle) != ATSTATUS_OK)
  {
    LogError(3, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  /* set adapter name for this context */
  at_context[affectedHandle].device_type = p_device_infos->type;
  at_context[affectedHandle].ipc_device  = p_device_infos->ipc_device;
  if (p_device_infos->ipc_interface == IPC_INTERFACE_UART)
  {
    at_context[affectedHandle].ipc_mode = IPC_MODE_UART_CHARACTER;
  }
  else
  {
    /* Only UART is supported */
    LogError(4, ERROR_FATAL);
    return (ATSTATUS_ERROR);
  }

  /* start in COMMAND MODE */
  at_context[affectedHandle].in_data_mode = AT_FALSE;

  /* no command actually in progress */
  at_context[affectedHandle].processing_cmd = 0U;

  /* init semaphore for data sent indication */
  at_context[affectedHandle].dataSent = AT_FALSE;

#if (RTOS_USED == 1)
  mySemaphoreDef(ATCORE_SEM_SEND, affectedHandle);
  at_context[affectedHandle].s_SendConfirm_SemaphoreId = osSemaphoreCreate(mySemaphore(ATCORE_SEM_SEND, affectedHandle), ATCORE_SEM_SEND_COUNT);
  if (at_context[affectedHandle].s_SendConfirm_SemaphoreId == NULL)
  {
    PrintErr("SendSemaphoreId creation error for handle = %d", affectedHandle)
    LogError(5, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }
  /* init semaphore */
  osSemaphoreWait(at_context[affectedHandle].s_SendConfirm_SemaphoreId, osWaitForever);
#else /* RTOS_USED */
  register_event_callback[affectedHandle] = event_callback;
#endif /* RTOS_USED */

  /* register client callback for URC */
  register_URC_callback[affectedHandle] = urc_callback;

  /* init the ATParser */
  if (ATParser_init(&at_context[affectedHandle], &custom_checkEndOfMsgCallback) != ATSTATUS_OK)
  {
    LogError(6, ERROR_FATAL);
    return (ATSTATUS_ERROR);
  }

  /* open the IPC */
  IPC_open(&ipcHandleTab[affectedHandle],
           at_context[affectedHandle].ipc_device,
           at_context[affectedHandle].ipc_mode,
           msgReceivedCallback,
           msgSentCallback,
           custom_checkEndOfMsgCallback);

  return (affectedHandle);
}

at_status_t AT_close(at_handle_t athandle)
{
  at_status_t retval = ATSTATUS_OK;

  PrintAPI("enter AT_close()")

  register_URC_callback[athandle] = NULL;
#if (RTOS_USED == 0)
  register_event_callback[athandle] = NULL;
#endif

  IPC_close(&ipcHandleTab[athandle]);

  return (retval);
}

at_status_t AT_reset_context(at_handle_t athandle)
{
  at_status_t retval = ATSTATUS_ERROR;

  at_context[athandle].in_data_mode = AT_FALSE;
  at_context[athandle].processing_cmd = 0U;
  at_context[athandle].dataSent = AT_FALSE;

#if (RTOS_USED == 1)
  at_context[athandle].action_flags = ATACTION_RSP_NO_ACTION;
#endif

  /* reinit IPC channel and select our channel */
  if (IPC_reset(&ipcHandleTab[athandle]) == IPC_OK)
  {
    if (IPC_select(&ipcHandleTab[athandle]) == IPC_OK)
    {
      retval = ATSTATUS_OK;
    }
  }

  return (retval);
}

at_status_t AT_sendcmd(at_handle_t athandle, at_msg_t msg_in_id, at_buf_t *p_cmd_in_buf, at_buf_t *p_rsp_buf)
{
  /* Sends a service request message to the ATCore.
  *  This is a blocking function.
  *  It returns when the command is fully processed or a timeout expires.
  */
  at_status_t retval;

  PrintAPI("enter AT_sendcmd()")

  /* Check if a command is already ongoing for this handle */
  if (at_context[athandle].processing_cmd == 1U)
  {
    PrintErr("!!!!!!!!!!!!!!!!!! WARNING COMMAND IS UNDER PROCESS !!!!!!!!!!!!!!!!!!")
    retval = ATSTATUS_ERROR;
    LogError(7, ERROR_WARNING);
    goto exit_func;
  }

  /* initialize response buffer */
  memset((void *)p_rsp_buf, 0, ATCMD_MAX_BUF_SIZE);

  /* start to process this command */
  at_context[athandle].processing_cmd = 1U;

#if (RTOS_USED == 1)
  /* save ptr on response buffer */
  at_context[athandle].p_rsp_buf = p_rsp_buf;
#endif /* RTOS_USED */

  /* Check if current mode is DATA mode */
  if (at_context[athandle].in_data_mode == AT_TRUE)
  {
    /* Check if user command is DATA suspend */
    if (msg_in_id == SID_CS_DATA_SUSPEND)
    {
      /* restore IPC Command channel to send ESCAPE COMMAND */
      PrintDBG("<<< restore IPC COMMAND channel >>>")
      IPC_select(&ipcHandleTab[athandle]);
    }
  }
  /* check if trying to suspend DATA in command mode */
  else if (msg_in_id == SID_CS_DATA_SUSPEND)
  {
    retval = ATSTATUS_ERROR;
    LogError(8, ERROR_WARNING);
    PrintErr("DATA not active")
    goto exit_func;
  }
  else
  {
    /* nothing to do */
  }

  /* Process the user request */
  retval = ATParser_process_request(&at_context[athandle], msg_in_id, p_cmd_in_buf);
  if (retval != ATSTATUS_OK)
  {
    PrintDBG("AT_sendcmd error: process request")
    ATParser_abort_request(&at_context[athandle]);
    goto exit_func;
  }

  /* Start an AT command transaction */
  retval = process_AT_transaction(athandle, msg_in_id, p_rsp_buf);
  if (retval != ATSTATUS_OK)
  {
    PrintDBG("AT_sendcmd error: process AT transaction")
    /* retrieve and send error report if exist */
    ATParser_get_error(&at_context[athandle], p_rsp_buf);
    ATParser_abort_request(&at_context[athandle]);
    goto exit_func;
  }

  /* get command response buffer */
  ATParser_get_rsp(&at_context[athandle], p_rsp_buf);

exit_func:
  /* finished to process this command */
  at_context[athandle].processing_cmd = 0U;

  return (retval);
}

#if (RTOS_USED == 0)
at_status_t  AT_getevent(at_handle_t athandle, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK, retUrc;
  at_action_rsp_t action;

  PrintAPI("enter AT_getevent()")

  /* retrieve response */
  if (IPC_receive(&ipcHandleTab[athandle], &msgFromIPC[athandle]) == IPC_ERROR)
  {
    PrintErr("IPC receive error")
    LogError(9, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  /* one message has been read */
  IRQ_DISABLE();
  MsgReceived[athandle]--;
  IRQ_ENABLE();

  /* Parse the response */
  action = ATParser_parse_rsp(&at_context[athandle], &msgFromIPC[athandle]);

  PrintDBG("RAW ACTION (get_event) = 0x%x", action)

  /* continue if this is an intermediate response */
  if ((action != ATACTION_RSP_URC_IGNORED) &&
      (action != ATACTION_RSP_URC_FORWARDED))
  {
    PrintINFO("this is not an URC (%d)", action);
    return (ATSTATUS_ERROR);
  }

  if (action == ATACTION_RSP_URC_FORWARDED)
  {
    /* notify user with callback */
    if (register_URC_callback[athandle] != NULL)
    {
      /* get URC response buffer */
      do
      {
        retUrc = ATParser_get_urc(&at_context[athandle], p_rsp_buf);
        if ((retUrc == ATSTATUS_OK) || (retUrc == ATSTATUS_OK_PENDING_URC))
        {
          /* call the URC callback */
          (* register_URC_callback[athandle])(p_rsp_buf);
        }
      }
      while (retUrc == ATSTATUS_OK_PENDING_URC);
    }
  }

  return (retval);
}
#endif /* RTOS_USED */

/* Private function Definition -----------------------------------------------*/
static uint8_t find_index(IPC_Handle_t *ipcHandle)
{
  uint8_t idx = 0U, found = 0U, retval = 0U;

  do
  {
    if (&ipcHandleTab[idx] == ipcHandle)
    {
      found = 1U;
      retval = idx;
    }
    idx++;
  }
  while ((idx <= ATCORE_MAX_HANDLES) && (found == 0U));

  if (found == 0U)
  {
    LogError(10, ERROR_FATAL);
    return (ATSTATUS_ERROR);
  }
  return (retval);
}

static void msgReceivedCallback(IPC_Handle_t *ipcHandle)
{
  /* Warning ! this function is called under IT */
  uint8_t index = find_index(ipcHandle);

#if (RTOS_USED == 1)
  /* disable irq not required, we are under IT */
  MsgReceived[index]++;

  if (osMessagePut(q_msg_IPC_received_Id, SIG_IPC_MSG, 0U) != osOK)
  {
    PrintErr("q_msg_IPC_received_Id error for SIG_IPC_MSG")
  }

#else
  MsgReceived[index]++;
  /* received during in IDLE state ? this is an URC */
  if (at_context[index].processing_cmd == 0)
  {
    if (register_event_callback[index] != NULL)
    {
      (* register_event_callback[index])();
    }
  }
#endif /* RTOS_USED */
}

static void msgSentCallback(IPC_Handle_t *ipcHandle)
{
  /* Warning ! this function is called under IT */
  uint8_t index = find_index(ipcHandle);
  at_context[index].dataSent = AT_TRUE;

#if (RTOS_USED == 1)
  osSemaphoreRelease(at_context[index].s_SendConfirm_SemaphoreId);
#endif

}

static at_status_t allocate_ATHandle(uint16_t *athandle)
{
  at_status_t retval = ATSTATUS_OK;
  uint8_t idx = 0U;

  /* find first free handle */
  while ((idx < ATCORE_MAX_HANDLES) && (at_context[idx].device_type != DEVTYPE_INVALID))
  {
    idx++;
  }

  if (idx == ATCORE_MAX_HANDLES)
  {
    LogError(11, ERROR_FATAL);
    retval = ATSTATUS_ERROR;
  }
  *athandle = idx;

  return (retval);
}

static at_status_t waitOnMsgUntilTimeout(uint16_t athandle, uint32_t Tickstart, uint32_t Timeout)
{
#if (RTOS_USED == 1)
  /*UNUSED(athandle);*/
  /*UNUSED(Tickstart);*/
#if (DBG_REQUEST_DURATION == 1)
  uint32_t tick_init = osKernelSysTick();
#endif /* DBG_REQUEST_DURATION */
  PrintDBG("**** Waiting Sema (to=%lu) *****", Timeout)
  if (osSemaphoreWait(s_WaitAnswer_SemaphoreId, Timeout) != osOK)
  {
    PrintDBG("**** Sema Timeout (=%ld) !!! *****", Timeout)
    return (ATSTATUS_TIMEOUT);
  }
#if (DBG_REQUEST_DURATION == 1)
  PrintINFO("Request duration = %d ms", (osKernelSysTick() - tick_init));
#endif /* DBG_REQUEST_DURATION */
  PrintDBG("**** Sema Freed *****")

#else
  /* Wait until flag is set */
  while (!(MsgReceived[athandle]))
  {
    /* Check for the Timeout */
    if (Timeout != ATCMD_MAX_DELAY)
    {
      if ((Timeout == 0) || ((HAL_GetTick() - Tickstart) > Timeout))
      {
        return (ATSTATUS_TIMEOUT);
      }
    }
  }
#endif /* RTOS_USED */
  return (ATSTATUS_OK);
}

static at_status_t process_AT_transaction(at_handle_t athandle, at_msg_t msg_in_id, at_buf_t *p_rsp_buf)
{
  /*UNUSED(p_rsp_buf);*/

  at_status_t retval;
  uint32_t tickstart;
  uint32_t at_cmd_timeout = 0U;
  at_action_send_t action_send;
  at_action_rsp_t action_rsp;
  uint16_t build_atcmd_size = 0U;
  uint8_t another_cmd_to_send;

  /* reset at cmd buffer */
  memset((void *) build_atcmd, 0, ATCMD_MAX_CMD_SIZE);

  do
  {
    memset((void *)&build_atcmd[0], 0, sizeof(AT_CHAR_t) * ATCMD_MAX_CMD_SIZE);
    build_atcmd_size = 0U;

    /* Get command to send */
    action_send = ATParser_get_ATcmd(&at_context[athandle],
                                     (uint8_t *)&build_atcmd[0],
                                     &build_atcmd_size, &at_cmd_timeout);
    if ((action_send & ATACTION_SEND_ERROR) != 0U)
    {
      PrintDBG("AT_sendcmd error: get at command")
      LogError(12, ERROR_WARNING);
      return (ATSTATUS_ERROR);
    }

    /* Send AT command through IPC if a valid command is available */
    if (build_atcmd_size > 0U)
    {
      /* Before to send a command, check if current mode is DATA mode
      *  (exception if request is to suspend data mode)
      */
      if ((at_context[athandle].in_data_mode == AT_TRUE) && (msg_in_id != SID_CS_DATA_SUSPEND))
      {
        /* impossible to send a CMD during data mode */
        PrintErr("DATA ongoing, can not send a command")
        LogError(13, ERROR_WARNING);
        return (ATSTATUS_ERROR);
      }

      retval = sendToIPC(athandle, (uint8_t *)&build_atcmd[0], build_atcmd_size);
      if (retval != ATSTATUS_OK)
      {
        PrintErr("AT_sendcmd error: send to ipc")
        LogError(14, ERROR_WARNING);
        return (ATSTATUS_ERROR);
      }
    }

    /* Wait for a response or a temporation (which could be = 0)*/
    if (((action_send & ATACTION_SEND_WAIT_MANDATORY_RSP) != 0U) ||
        ((action_send & ATACTION_SEND_TEMPO) != 0U))
    {
      /* init tickstart for a full AT transaction */
      tickstart = HAL_GetTick();

      do
      {
        /* Wait for response from IPC */
        retval = waitFromIPC(athandle, tickstart, at_cmd_timeout, &msgFromIPC[athandle]);
        if (retval != ATSTATUS_OK)
        {
          if ((action_send & ATACTION_SEND_WAIT_MANDATORY_RSP) != 0U)
          {
            /* Waiting for a response (mandatory) */
            if (retval == ATSTATUS_TIMEOUT)
            {
              /* in case of advanced debug
               IPC_dump_RX_queue(&ipcHandleTab[athandle], 1);
              */
            }
            PrintErr("AT_sendcmd error: wait from ipc")
#if (DBG_REQUEST_DURATION == 1)
            PrintErr("Interrupts count: disable=%d enable=%d",
                     count_disable,
                     count_enable)
#endif /* DBG_REQUEST_DURATION */
            LogError(15, ERROR_WARNING);
            return (ATSTATUS_ERROR);
          }
          else /* ATACTION_SEND_TEMPO */
          {
            /* Temporisation (was waiting for a non-mandatory event)
            *  now that timer has expired, proceed to next action if needed
            */
            if ((action_send & ATACTION_SEND_FLAG_LAST_CMD) != 0U)
            {
              action_rsp = ATACTION_RSP_FRC_END;
            }
            else
            {
              action_rsp = ATACTION_RSP_FRC_CONTINUE;
            }
            goto continue_execution;
          }
        }

#if (RTOS_USED == 1)
        /* Retrieve the action which has been set on IPC msg reception in ATCoreTaskBody
        *  More than one action could has been set
        */
        if ((at_context[athandle].action_flags & ATACTION_RSP_FRC_END) != 0U)
        {
          action_rsp = ATACTION_RSP_FRC_END;
          /* clean flag */
          at_context[athandle].action_flags &= ~((at_action_rsp_t) ATACTION_RSP_FRC_END);
        }
        else if ((at_context[athandle].action_flags & ATACTION_RSP_FRC_CONTINUE) != 0U)
        {
          action_rsp = ATACTION_RSP_FRC_CONTINUE;
          /* clean flag */
          at_context[athandle].action_flags &= ~((at_action_rsp_t) ATACTION_RSP_FRC_CONTINUE);
        }
        else if ((at_context[athandle].action_flags & ATACTION_RSP_ERROR) != 0U)
        {
          action_rsp = ATACTION_RSP_ERROR;
          /* clean flag */
          at_context[athandle].action_flags &= ~((at_action_rsp_t) ATACTION_RSP_ERROR);
          PrintDBG("AT_sendcmd error: parse from rsp")
          LogError(16, ERROR_WARNING);
          return (ATSTATUS_ERROR);
        }
        else
        {
          /* all other actions are ignored */
          action_rsp = ATACTION_RSP_IGNORED;
        }
#else
        /* Parse the response */
        action_rsp = ATParser_parse_rsp(&at_context[athandle], &msgFromIPC[athandle]);

        /* analyze the response (check data mode flag) */
        action_rsp = analyze_action_result(athandle, action_rsp);
        PrintDBG("RAW ACTION (AT_sendcmd) = 0x%x", action_rsp)
        if (action_rsp == ATACTION_RSP_ERROR)
        {
          PrintDBG("AT_sendcmd error: parse from rsp")
          LogError(17, ERROR_WARNING);
          return (ATSTATUS_ERROR);
        }
#endif /* RTOS_USED */

        /* continue if this is an intermediate response */
continue_execution:

        PrintDBG("")

#if (RTOS_USED == 0)
        /* URC received during AT transaction */
        if (action_rsp == ATACTION_RSP_URC_FORWARDED)
        {
          /* notify user with callback */
          PrintDBG("---> URC FORWARDED !!!")
          if (register_URC_callback[athandle] != NULL)
          {
            at_status_t retUrc;

            do
            {
              /* get URC response buffer */
              retUrc = ATParser_get_urc(&at_context[athandle], p_rsp_buf);
              if ((retUrc == ATSTATUS_OK) || (retUrc == ATSTATUS_OK_PENDING_URC))
              {
                /* call the URC callback */
                (* register_URC_callback[athandle])(p_rsp_buf);
              }
            }
            while (retUrc == ATSTATUS_OK_PENDING_URC);
          }
          else
          {
            PrintErr("No valid callback to forward URC")
          }
        }
#endif /* RTOS_USED */

      }
      while ((action_rsp == ATACTION_RSP_INTERMEDIATE) ||
             (action_rsp == ATACTION_RSP_IGNORED) ||
             (action_rsp == ATACTION_RSP_URC_FORWARDED) ||
             (action_rsp == ATACTION_RSP_URC_IGNORED));

      if (action_rsp == ATACTION_RSP_FRC_CONTINUE)
      {
        another_cmd_to_send = 1U;
      }
      else
      {
        /* FRC_END, ERRORS,... */
        another_cmd_to_send = 0U;
      }
    }
    else
    {
      PrintErr("Invalid action code")
      LogError(18, ERROR_WARNING);
      return (ATSTATUS_ERROR);
    }

  }
  while (another_cmd_to_send == 1U);

#if (RTOS_USED == 1)
  /* clear all flags*/
  at_context[athandle].action_flags = ATACTION_RSP_NO_ACTION;
#endif

  PrintDBG("action_rsp value = %d", action_rsp)
  if (action_rsp == ATACTION_RSP_ERROR)
  {
    LogError(19, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  return (ATSTATUS_OK);
}


static at_status_t sendToIPC(at_handle_t athandle,
                             uint8_t *cmdBuf, uint16_t cmdSize)
{
  PrintAPI("enter sendToIPC()")

  /* Send AT command */
  if (IPC_send(&ipcHandleTab[athandle], cmdBuf, cmdSize) == IPC_ERROR)
  {
    PrintErr(" IPC send error")
    LogError(20, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

#if (RTOS_USED == 1)
  osSemaphoreWait(at_context[athandle].s_SendConfirm_SemaphoreId, osWaitForever);
#else
  /* waiting TX confirmation, done by callback from IPC */
  while (at_context[athandle].dataSent == AT_FALSE)
  {
  }
  at_context[athandle].dataSent = AT_FALSE;
#endif /* RTOS_USED */

  return (ATSTATUS_OK);
}

static at_status_t waitFromIPC(at_handle_t athandle,
                               uint32_t tickstart, uint32_t cmdTimeout, IPC_RxMessage_t *p_msg)
{
  /*UNUSED(p_msg);*/

  at_status_t retval;
  PrintAPI("enter waitFromIPC()")

  /* wait for complete message */
  retval = waitOnMsgUntilTimeout(athandle, tickstart, cmdTimeout);
  if (retval != ATSTATUS_OK)
  {
    if (cmdTimeout != 0U)
    {
      PrintINFO("TIMEOUT EVENT(%ld ms)", cmdTimeout)
    }
    else
    {
      /* PrintINFO("No timeout"); */
    }
    return (retval);
  }

#if (RTOS_USED == 0)
  /* retrieve response */
  if (IPC_receive(&ipcHandleTab[athandle], p_msg) == IPC_ERROR)
  {
    PrintErr("IPC receive error")
    LogError(21, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  /* one message has been read */
  IRQ_DISABLE();
  MsgReceived[athandle]--;
  IRQ_ENABLE();
#endif /* RTOS_USED */
  return (ATSTATUS_OK);
}

static at_action_rsp_t analyze_action_result(at_handle_t athandle, at_action_rsp_t val)
{
  at_action_rsp_t action;

  /* retrieve DATA flag value */
  at_bool_t data_mode = ((val & ATACTION_RSP_FLAG_DATA_MODE) != 0U) ? AT_TRUE : AT_FALSE;

  /* clean DATA flag value */
  action = (at_action_rsp_t)(val & ~(at_action_rsp_t)ATACTION_RSP_FLAG_DATA_MODE);

  PrintDBG("RAW ACTION (analyze_action_result) = 0x%x", val)
  PrintDBG("CLEANED ACTION=%d (data mode=%d)", action, data_mode)

  if (data_mode == AT_TRUE)
  {
    /* DATA MODE */
    if (at_context[athandle].in_data_mode == AT_FALSE)
    {
      IPC_Handle_t *h_other_ipc = IPC_get_other_channel(&ipcHandleTab[athandle]);
      if (h_other_ipc != NULL)
      {
        IPC_select(h_other_ipc);
        at_context[athandle].in_data_mode = AT_TRUE;
        PrintINFO("<<< DATA MODE SELECTED >>>")
      }
      else
      {
        PrintErr("<<< ERROR WHEN SELECTING DATA MODE >>>")
        action = ATACTION_RSP_ERROR;
      }
    }
  }
  else
  {
    /* COMMAND MODE */
    if (at_context[athandle].in_data_mode == AT_TRUE)
    {
      at_context[athandle].in_data_mode = AT_FALSE;

      PrintINFO("<<< COMMAND MODE SELECTED >>>")
    }
  }

  /* return action cleaned from DATA flag value */
  return (action);
}

static void IRQ_DISABLE(void)
{
#if (DBG_REQUEST_DURATION == 1)
  count_disable = (count_disable + 1) % COUNT_MAX_VAL;
#endif /* DBG_REQUEST_DURATION */
  __disable_irq();
}

static void IRQ_ENABLE(void)
{
#if (DBG_REQUEST_DURATION == 1)
  count_enable = (count_enable + 1) % COUNT_MAX_VAL;
#endif /* DBG_REQUEST_DURATION */
  __enable_irq();
}

#if (RTOS_USED == 1)
at_status_t atcore_task_start(osPriority taskPrio, uint32_t stackSize)
{
  /* check if AT_init has been called before */
  if (AT_Core_initialized != 1U)
  {
    PrintErr("error, ATCore is not initialized")
    LogError(22, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }

  /* semaphores creation */
  osSemaphoreDef(ATCORE_SEM_WAIT_ANSWER);
  s_WaitAnswer_SemaphoreId = osSemaphoreCreate(osSemaphore(ATCORE_SEM_WAIT_ANSWER), ATCORE_SEM_WAIT_ANSWER_COUNT);
  if (s_WaitAnswer_SemaphoreId == NULL)
  {
    PrintErr("s_WaitAnswer_SemaphoreId creation error")
    LogError(23, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }
  /* init semaphore */
  osSemaphoreWait(s_WaitAnswer_SemaphoreId, osWaitForever);

  /* queues creation */
  osMessageQDef(IPC_MSG_RCV, MSG_IPC_RECEIVED_SIZE, uint16_t); /* Define message queue */
  q_msg_IPC_received_Id = osMessageCreate(osMessageQ(IPC_MSG_RCV), NULL); /* create message queue */

  /* start driver thread */
  osThreadDef(atcoreTask, ATCoreTaskBody, taskPrio, 0, stackSize);
  atcoreTaskId = osThreadCreate(osThread(atcoreTask), NULL);
  if (atcoreTaskId == NULL)
  {
    PrintErr("atcoreTaskId creation error")
    LogError(24, ERROR_WARNING);
    return (ATSTATUS_ERROR);
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(atcoreTaskId, stackSize);
#endif /* STACK_ANALYSIS_TRACE */
  }

  return (ATSTATUS_OK);
}

static at_status_t findMsgReceivedHandle(at_handle_t *athandle)
{
  for (uint16_t i = 0U; i < ATCORE_MAX_HANDLES; i++)
  {
    if (MsgReceived[i] != 0U)
    {
      *athandle = (uint16_t)i;
      return ATSTATUS_OK;
    }
  }
  LogError(25, ERROR_WARNING);
  return ATSTATUS_ERROR;
}

static void ATCoreTaskBody(void const *argument)
{
  /*UNUSED(argument);*/

  at_status_t retUrc;
  at_handle_t athandle;
  at_status_t ret;
  at_action_rsp_t action;
  osEvent event;

  PrintAPI("<start ATCore TASK>")

  /* Infinite loop */
  for (;;)
  {
    /* waiting IPC message received event (message) */
    event = osMessageGet(q_msg_IPC_received_Id, osWaitForever);
    if (event.status == osEventMessage)
    {
      PrintDBG("<ATCore TASK> - received msg event= 0x%lx", event.value.v)

      if (event.value.v == (SIG_IPC_MSG))
      {
        /* a message has been received, retrieve its handle */
        ret = findMsgReceivedHandle(&athandle);
        if (ret != ATSTATUS_OK)
        {
          /* skip this loop iteration */
          continue;
        }

        /* retrieve message from IPC */
        if (IPC_receive(&ipcHandleTab[athandle], &msgFromIPC[athandle]) == IPC_ERROR)
        {
          PrintErr("IPC receive error")
          ATParser_abort_request(&at_context[athandle]);
          PrintDBG("**** Sema Realesed on error 1 *****")
          osSemaphoreRelease(s_WaitAnswer_SemaphoreId);
          /* skip this loop iteration */
          continue;
        }

        /* one message has been read */
        IRQ_DISABLE();
        MsgReceived[athandle]--;
        IRQ_ENABLE();

        /* Parse the response */
        action = ATParser_parse_rsp(&at_context[athandle], &msgFromIPC[athandle]);

        /* analyze the response (check data mode flag) */
        action = analyze_action_result(athandle, action);

        /* add this action to action flags */
        at_context[athandle].action_flags |= action;
        PrintDBG("add action 0x%x (flags=0x%x)", action, at_context[athandle].action_flags)
        if (action == ATACTION_RSP_ERROR)
        {
          PrintErr("AT_sendcmd error")
          ATParser_abort_request(&at_context[athandle]);
          PrintDBG("**** Sema Realesed on error 2 *****")
          osSemaphoreRelease(s_WaitAnswer_SemaphoreId);
          continue;
        }

        /* check if this is an URC to forward */
        if (action == ATACTION_RSP_URC_FORWARDED)
        {
          /* notify user with callback */
          if (register_URC_callback[athandle] != NULL)
          {
            /* get URC response buffer */
            do
            {
              retUrc = ATParser_get_urc(&at_context[athandle], at_context[athandle].p_rsp_buf);
              if ((retUrc == ATSTATUS_OK) || (retUrc == ATSTATUS_OK_PENDING_URC))
              {
                /* call the URC callback */
                (* register_URC_callback[athandle])(at_context[athandle].p_rsp_buf);
              }
            }
            while (retUrc == ATSTATUS_OK_PENDING_URC);
          }
        }
        else if ((action == ATACTION_RSP_FRC_CONTINUE) ||
                 (action == ATACTION_RSP_FRC_END) ||
                 (action == ATACTION_RSP_ERROR))
        {
          PrintDBG("**** Sema released *****")
          osSemaphoreRelease(s_WaitAnswer_SemaphoreId);
        }
        else
        {
          /* nothing to do */
        }

      }
    }
  }
}
#endif /* RTOS_USED */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

