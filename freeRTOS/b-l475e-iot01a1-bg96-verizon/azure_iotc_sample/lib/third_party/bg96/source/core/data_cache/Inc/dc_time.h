/**
  ******************************************************************************
  * @file    dc_service.h
  * @author  MCD Application Team
  * @brief   Header for dc_service.c module
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

#ifndef DC_SERVICE_H
#define DC_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "dc_common.h"
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern dc_com_res_id_t    DC_COM_TIME_DATE ;

/* Exported macros -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  DC_TIME,
  DC_DATE,
  DC_DATE_AND_TIME
} dc_time_data_type_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  uint32_t sec;    /* seconds 0-61 */
  uint32_t min;    /* minutes 0-59 */
  uint32_t hour;   /* hours 0-23 */
  uint32_t mday;   /* day of the month  1-31   (based on RTC HAL values)*/
  uint32_t month;  /* month since january 1-12 (based on RTC HAL values) */
  uint32_t year;   /* year since 1970 */
  uint32_t wday;   /* day since monday 1-7 (based on RTC HAL values) */
  uint32_t yday;   /* days since January 1 */
  uint32_t isdst;  /* daylight saving time 0-365 */
}   dc_time_date_rt_info_t;


/* dc_srv_target_service_state_t:  used to set the target service state */
typedef enum
{
  DC_SERVICE_TARGET_RESET,   /* see dc_srv_service_reset_param_t for detail */
  DC_SERVICE_TARGET_RESTORE, /* restore default setting. For example, if the Service is by default OFF then it will power OFF the device. Default value are stored in FW. */
  DC_SERVICE_TARGET_CALIB,   /* calibrate */
  DC_SERVICE_TARGET_OFF,     /* service is powered down but related data remains in data cache. They can be retrieved by GUI when needed. */
  DC_SERVICE_TARGET_ON       /* start the service. The actual service could be ON or READY see DC.  */
} dc_srv_target_service_state_t;


typedef struct
{
  uint32_t time_update_period; /* in milliseconds, 0 means default value defined in TIME Service or from NVRAM */
  uint8_t time_setting_mode;   /* 0 means manual setting. This is the default setting.
                                    1 full automatic, based on UTC TIme server and location (GPS based or MNO)
                                    2 semi automatic updated from UTC Network Time Server (example via Network Time Server)
                                    3 automatic update via BLE */
  uint8_t utc_offset; /* used only if time is set with time_setting_mode = 2 */
} dc_srv_time_params_t;

typedef struct
{
  uint32_t volume_update_period; /* in milliseconds, 0 means default value defined in CS Service */
  uint32_t signal_strength_update_period; /* in milliseconds, 0 means default value defined in the CS Service */
} dc_srv_cellular_params_t;

typedef struct
{
  uint8_t mode; /* 0 means RTC not preserved in shutdown state */
  /* 1 means RTC is preserved in shutdown state */
} dc_srv_device_off_params_t;



typedef enum
{
  DC_SRV_RET_OK,
  DC_SRV_RET_ERROR
} dc_srv_ret_t;


/* Exported functions ------------------------------------------------------- */

/**
  * @brief  set system date and time
  * @param  dc_time_date_rt_info     (in) date to set
  * @param  dc_time_data_type_t      (in) time to set
  * @retval dc_srv_ret_t             return status
  */

dc_srv_ret_t dc_srv_get_time_date(dc_time_date_rt_info_t *dc_time_date_rt_info, dc_time_data_type_t time_date);

/**
  * @brief  get system date and time
  * @param  dc_time_date_rt_info     (out) date
  * @param  dc_time_data_type_t      (out) time
  * @retval dc_srv_ret_t             return status
  */

dc_srv_ret_t dc_srv_set_time_date(const dc_time_date_rt_info_t *time, dc_time_data_type_t time_date);


/**
  * @brief  init system date and time data cache entry
  */
void dc_srv_init(void);

#ifdef __cplusplus
}
#endif



#endif /* __DC_SERVICE_H */

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/
