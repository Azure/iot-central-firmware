/**
  ******************************************************************************
  * @file    dc_common.c
  * @author  MCD Application Team
  * @brief   Code of Data Cache common services
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
#include "dc_common.h"
#include "dc_control.h"
#include "dc_time.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
dc_com_db_t dc_com_db;

/* Private function prototypes -----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/



/**
  * @brief  to register a generic cb to a given dc_db
  * @param  dc_db               data base reference
  * @param  notif_cb            user callback
  * @param  private_gui_data    user context
  * @retval dc_com_reg_id_t     created entry identifier
  */
dc_com_reg_id_t dc_com_register_gen_event_cb(dc_com_db_t *dc_db, dc_com_gen_event_callback_t notif_cb, void *private_gui_data)
{
  dc_com_reg_id_t user_id;
  if (dc_db->user_number < DC_COM_MAX_NB_USERS)
  {
    user_id = dc_com_db.user_number;
    dc_db->user_info[user_id].user_reg_id       = user_id;
    dc_db->user_info[user_id].notif_cb          = notif_cb;
    dc_db->user_info[user_id].private_user_data = private_gui_data;
    dc_db->user_number++;
  }
  else
  {
    user_id = -1;
  }

  return user_id;
}

/**
  * @brief  to register ser service in dc
  * @param  dc_db               (in) data base reference
  * @param  data                (in) data structure
  * @param  len                 (in) data len
  * @retval res_id              (out) res id
  */
dc_com_res_id_t dc_com_register_serv(dc_com_db_t *dc_db,  void *data, uint16_t len)
{
  dc_com_res_id_t res_id;
  dc_base_rt_info_t *base_rt;

  if (dc_db->serv_number < DC_COM_SERV_MAX)
  {
    res_id = dc_com_db.serv_number;
    dc_db->dc_db[dc_com_db.serv_number]     = data;
    dc_db->dc_db_len[dc_com_db.serv_number] = len;
    base_rt = (dc_base_rt_info_t *)data;

    base_rt->header.res_id = res_id;
    base_rt->header.size   = len;
    base_rt->rt_state      = DC_SERVICE_OFF;

    dc_db->serv_number++;
  }
  else
  {
    res_id = -1;
  }

  return res_id;
}

/**
  * @brief  update a data info in the DC
  * @param  dc                  data base reference
  * @param  res_id              resource id
  * @param  data                data to write
  * @param  len                 len of data to write
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_write(void *dc, dc_com_res_id_t res_id, void *data, uint16_t len)
{
  dc_com_reg_id_t reg_id;
  dc_com_event_id_t event_id = (dc_com_event_id_t)res_id;
  dc_base_rt_info_t *dc_base_rt_info;

  dc_com_db_t *com_db = (dc_com_db_t *)dc;
  memcpy((void *)(com_db->dc_db[res_id]), data, (uint32_t)len);
  dc_base_rt_info = (dc_base_rt_info_t *)(com_db->dc_db[res_id]);
  dc_base_rt_info->header.res_id = (dc_com_res_id_t)event_id;
  dc_base_rt_info->header.size   = len;

  for (reg_id = 0; reg_id < DC_COM_MAX_NB_USERS; reg_id++)
  {
    if (com_db->user_info[reg_id].notif_cb != NULL)
    {
      com_db->user_info[reg_id].notif_cb(event_id, com_db->user_info[reg_id].private_user_data);
    }
  }


  return DC_COM_OK;
}

/**
  * @brief  read current data info in the DC
  * @param  dc                  data base reference
  * @param  res_id              resource id
  * @param  data                data to read
  * @param  len                 len of data to read
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_read(void *dc, dc_com_res_id_t res_id, void *data, uint16_t len)
{
  dc_com_db_t *com_db = (dc_com_db_t *)dc;
  memcpy(data, (void *)com_db->dc_db[res_id], (uint32_t)len);


  return DC_COM_OK;
}


/**
  * @brief  send an event to DC
  * @param  dc                  data base reference
  * @param  event_id            event id
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_write_event(void *dc, dc_com_event_id_t event_id)
{
  dc_com_reg_id_t reg_id;
  dc_com_db_t *com_db = (dc_com_db_t *)dc;

  for (reg_id = 0; reg_id < DC_COM_MAX_NB_USERS; reg_id++)
  {
    if (com_db->user_info[reg_id].notif_cb != NULL)
    {
      com_db->user_info[reg_id].notif_cb(event_id, com_db->user_info[reg_id].private_user_data);
    }
  }
  return DC_COM_OK;
}

/**
  * @brief  create the dc and load the data cache to RAM (BACKUPSRAM) with default value if specific setting does not exist.
  * @param  init_db             input data base
  * @param  dc                  output data base
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_init(void)
{
  int32_t i;

  memset(&dc_com_db, 0, sizeof(dc_com_db));

  dc_com_db.user_number = 0;
  dc_com_db.serv_number = 0;

  /* user_reg default values init */
  for (i = 0 ; i < (int)DC_COM_MAX_NB_USERS; i++)
  {
    dc_com_db.user_info[dc_com_db.user_number].user_reg_id = 0;

    dc_com_db.user_info[i].user_reg_id         = 0;
    dc_com_db.user_info[i].notif_cb            = 0;
    dc_com_db.user_info[i].private_user_data   = 0;
  }

  for (i = 0 ; i < (int)DC_COM_SERV_MAX; i++)
  {
    dc_com_db.dc_db[i] = 0;
  }
  dc_ctrl_event_init();
  dc_srv_init();

  return DC_COM_OK;
}

/**
  * @brief  dc data base start
  * @param  none
  * @retval dc_main_status_t    return status
  */

void dc_com_start(void)
{
  /* In this function, we start all DC tasks */
  dc_ctrl_event_start();
}

/**
  * @brief  unload the DC from RAM or BACKUP SRAM (INIT IMPLEMENTED).
  * @param  init_db             input data base
  * @param  dc                  output data base
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_deinit(void *init_db, void *dc)
{
  return DC_COM_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

