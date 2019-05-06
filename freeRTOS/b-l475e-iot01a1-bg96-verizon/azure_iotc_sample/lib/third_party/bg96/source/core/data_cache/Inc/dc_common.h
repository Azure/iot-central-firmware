/**
  ******************************************************************************
  * @file    dc_common.h
  * @author  MCD Application Team
  * @brief   Header for dc_common.c module
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
#ifndef DC_COMMON_H
#define DC_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define DC_COM_MAX_NB_USERS 10
#define DC_COM_SERV_MAX     30

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef int16_t  dc_com_reg_id_t;
typedef int16_t  dc_com_res_id_t;
typedef int16_t  dc_com_event_id_t;


typedef enum
{
  DC_SERVICE_UNAVAIL = 0x00, /* Service is unavailable. HW and/or SW driver are not present. */
  DC_SERVICE_RESET,          /* Service is resetting. When reset is complete, the Service enters in ON or READY state */
  DC_SERVICE_CALIB,          /* Service is under calibration procedure */
  DC_SERVICE_OFF,            /* Service is OFF */
  DC_SERVICE_SHUTTING_DOWN,  /* Service is being shutdown */
  DC_SERVICE_STARTING,       /* Service is starting but not fully operational */
  DC_SERVICE_RUN,            /* Service is ON and fully operational and calibrated */
  DC_SERVICE_ON,             /* Service is ON (functional) but not Calibrated or not Initialized */
  DC_SERVICE_FAIL            /* Service is Failed */
} dc_service_rt_state_t;

typedef enum
{
  DC_COM_OK = 0x00,
  DC_COM_ERROR
} dc_com_status_t;

typedef enum
{
  DC_COM_REG_EVENT_OFF, /* Upper-layer not ready to the receive event notification from dc_com  */
  DC_COM_REG_EVENT_ON   /* Upper-layer is ready receive event notification from dc_com */
} dc_com_reg_state_notif_t;


typedef struct
{
  dc_com_res_id_t res_id;
  uint32_t size;
} dc_service_rt_header_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  uint8_t data;
} dc_base_rt_info_t;


typedef void (*dc_com_gen_event_callback_t)(
  const dc_com_event_id_t event_id,
  void *private_user_data);

typedef void (*dc_com_spe_event_callback_t)(
  void *data,
  void *private_user_data);

typedef struct
{
  dc_com_reg_state_notif_t state_notif;
  dc_com_spe_event_callback_t event_cb;
  uint32_t timestamp; /* time stamp based on system tick from local MCU */
  uint8_t pending_notif; /* to track if GUI has read pending notification */
} dc_com_reg_notif_t;

typedef struct
{
  dc_com_reg_id_t user_reg_id;
  dc_com_gen_event_callback_t notif_cb;
  void *private_user_data;
} dc_com_user_info_t;

typedef struct
{
  dc_com_reg_id_t   user_number;
  dc_com_res_id_t   serv_number;
  dc_com_user_info_t user_info[DC_COM_MAX_NB_USERS];
  void *dc_db[DC_COM_SERV_MAX];
  uint16_t dc_db_len[DC_COM_SERV_MAX];
} dc_com_db_t;



/* Exported vars ------------------------------------------------------------*/

extern dc_com_db_t dc_com_db;

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  to register a generic cb to a given dc_db
  * @param  dc_db               data base reference
  * @param  notif_cb            user callback
  * @param  private_gui_data    user context
  * @retval dc_com_reg_id_t     created entry identifier
  */

dc_com_reg_id_t dc_com_register_gen_event_cb(
  dc_com_db_t *dc_db,
  dc_com_gen_event_callback_t notif_cb, /* the user event callback */
  void *private_gui_data);          /* user private data */


/**
  * @brief  to register ser service in dc
  * @param  dc_db               (in) data base reference
  * @param  data                (in) data structure
  * @param  len                 (in) data len
  * @retval res_id              (out) res id
  */
dc_com_res_id_t dc_com_register_serv(dc_com_db_t *dc_db,  void *data, uint16_t len);

/**
  * @brief  create the dc and load the data cache to RAM (BACKUPSRAM) with default value if specific setting does not exist.
  * @param  init_db             input data base
  * @param  dc                  output data base
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_init(void);

/**
  * @brief  dc data base start
  * @param  none
  * @retval dc_main_status_t    return status
  */

void dc_com_start(void);

/**
  * @brief  unload the DC from RAM or BACKUP SRAM (INIT IMPLEMENTED).
  * @param  init_db             input data base
  * @param  dc                  output data base
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_deinit(void *init_db, void *dc);

/**
  * @brief  update a data info in the DC
  * @param  dc                  data base reference
  * @param  res_id              resource id
  * @param  data                data to write
  * @param  len                 len of data to write
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_write(void *dc, dc_com_res_id_t res_id, void *data, uint16_t len);

/**
  * @brief  read current data info in the DC
  * @param  dc                  data base reference
  * @param  res_id              resource id
  * @param  data                data to read
  * @param  len                 len of data to read
  * @retval dc_com_status_t     return status
  */
dc_com_status_t dc_com_read(void *dc, dc_com_res_id_t res_id, void *data, uint16_t len);

/**
  * @brief  send an event to DC
  * @param  dc                  data base reference
  * @param  event_id            event id
  * @retval dc_com_status_t     return status
  */

dc_com_status_t dc_com_write_event(void *dc, dc_com_event_id_t event_id);

#ifdef __cplusplus
}
#endif


#endif /* DC_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
