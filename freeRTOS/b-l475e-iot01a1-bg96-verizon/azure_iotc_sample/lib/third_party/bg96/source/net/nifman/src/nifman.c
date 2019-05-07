/**
  ******************************************************************************
  * @file    nifman.c
  * @author  MCD Application Team
  * @brief   This file contains the Network InterFace MANager code
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
#include <stdint.h>
#include "cmsis_os.h"
#include "dc_common.h"
#include "cellular_init.h"
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif_client.h"
#endif
#include "error_handler.h"
#include "main.h"
#include "nifman.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static osMessageQId nifman_queue;

/* Global variables ----------------------------------------------------------*/
osThreadId  nifman_ThreadId = NULL;

/* Functions Definition ------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void   nifman_notif_cb(dc_com_event_id_t dc_event_id, void *private_gui_data);
static void nifman_thread(void const *argument);

/* Data cache callback ------------------------------------------------------- */
static void   nifman_notif_cb(dc_com_event_id_t dc_event_id, void *private_gui_data)
{
  if ((dc_event_id == DC_COM_CELLULAR_DATA_INFO)
      || (dc_event_id == DC_COM_PPP_CLIENT_INFO)
     )
  {
    osMessagePut(nifman_queue, (uint32_t)dc_event_id, 0U);
  }
}


/* nifman_thread function */
static void nifman_thread(void const *argument)
{
  osEvent event;
  dc_com_event_id_t          dc_event_id;
  dc_cellular_data_info_t    cellular_data_info;
  dc_ppp_client_info_t       ppp_client_info;
  dc_nifman_info_t           nifman_info;


  /* Modem activated : start ppp client config */

  uint32_t err;
  err  = 0U;

  if (err != 0U)
  {
    ERROR_Handler(DBG_CHAN_NIFMAN, 1, ERROR_FATAL);
  }

  for (;;)
  {
    event = osMessageGet(nifman_queue, osWaitForever);
    dc_event_id = (dc_com_event_id_t)event.value.v;

    if (dc_event_id == DC_COM_CELLULAR_DATA_INFO)
    {
      dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
      switch (cellular_data_info.rt_state)
      {
        case DC_SERVICE_ON:
        {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
          /* MODEM CELLULAR DATA Ready : PPPOS to configure */
          err = ppposif_client_config();
          if (err != 0U)
          {
            ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
            cellular_data_info.rt_state   =  DC_SERVICE_FAIL;
            dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
          }
#else
          /* MODEM CELLULAR DATA operational */
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  cellular_data_info.rt_state;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
#endif
          break;
        }
        case DC_SERVICE_SHUTTING_DOWN:
        {
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_SHUTTING_DOWN;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        case DC_SERVICE_FAIL:
        {
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_FAIL;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        case DC_SERVICE_OFF:
        {
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        default:
        {
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
      }
    }
    else if (dc_event_id == (dc_com_event_id_t)DC_COM_PPP_CLIENT_INFO)
    {
      dc_com_read(&dc_com_db, DC_COM_PPP_CLIENT_INFO, (void *)&ppp_client_info, sizeof(ppp_client_info));
      switch (ppp_client_info.rt_state)
      {
        case DC_SERVICE_ON:
        {
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_ON;
          nifman_info.network    =  DC_CELLULAR_NETWORK;
          nifman_info.ip_addr    =  ppp_client_info.ip_addr;
          nifman_info.gw         =  ppp_client_info.gw;
          nifman_info.netmask    =  ppp_client_info.netmask;

          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        default:
        {
          /* inform application to reset automaton */
          dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));

          /* inform cellular service task to reset automaton */
          dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
          cellular_data_info.rt_state   =  DC_SERVICE_FAIL;
          dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));

          break;
        }
      }
    }
    else
    {
      /* Nothing to do */
    }
  }
}

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  component init
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_init(void)
{
  nifman_status_t ret;
  osMessageQDef(nifman_queue, 1, uint32_t);
  nifman_queue = osMessageCreate(osMessageQ(nifman_queue), NULL);
  if (nifman_queue == NULL)
  {
    ret = NIFMAN_ERROR;
    ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
  }
  else
  {
    ret = NIFMAN_OK;
  }

  return ret;
}

/**
  * @brief  component start
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_start(void)
{
  nifman_status_t ret;
  dc_com_register_gen_event_cb(&dc_com_db, nifman_notif_cb, (void *) NULL);

  osThreadDef(NIFMAN, nifman_thread, NIFMAN_THREAD_PRIO, 0, NIFMAN_THREAD_STACK_SIZE);
  nifman_ThreadId = osThreadCreate(osThread(NIFMAN), NULL);
  if (nifman_ThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
    ret =  NIFMAN_ERROR;
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(nifman_ThreadId, NIFMAN_THREAD_STACK_SIZE);
#endif /* STACK_ANALYSIS_TRACE */
    ret = NIFMAN_OK;
  }

  return ret;
}

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/
