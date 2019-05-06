/**
  ******************************************************************************
  * @file    dc_service.c
  * @author  MCD Application Team
  * @brief   This file contains data cache services
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
#include "plf_config.h"
#include "rtc.h"
#include "dc_common.h"
#include "dc_time.h"

#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint16_t month_day[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

dc_time_date_rt_info_t dc_time_date;

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t DC_COM_TIME_DATE = -1;

/* Private function prototypes -----------------------------------------------*/

static uint16_t get_yday(RTC_DateTypeDef *Date);

/* Functions Definition ------------------------------------------------------*/

/* Private Functions Definition ------------------------------------------------------*/
static uint16_t get_yday(RTC_DateTypeDef *Date)
{
  uint16_t i;
  uint16_t yday;
  yday = 0U;

  for (i = 1U; i < Date->Month; i++)
  {
    yday += month_day[i];
  }

  yday += Date->Date;

  if (((Date->Year % 4U) == 0U) && (Date->Month > 1U))
  {
    yday++;
  }

  return yday;
}

/* Exported Functions Definition ------------------------------------------------------*/

/**
  * @brief  set system date and time
  * @param  dc_time_date_rt_info     (in) date to set
  * @param  dc_time_data_type_t      (in) time to set
  * @retval dc_srv_ret_t             return status
  */

dc_srv_ret_t dc_srv_set_time_date(const dc_time_date_rt_info_t *time, dc_time_data_type_t time_date)
{
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  dc_srv_ret_t ret;
  ret = DC_SRV_RET_OK;

  if (time_date != DC_DATE)
  {

    sTime.Hours   = (uint8_t)time->hour;
    sTime.Minutes = (uint8_t)time->min;
    sTime.Seconds = (uint8_t)time->sec;

    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
      if (time_date != DC_TIME)
      {
        if ((time->wday > 7U) || (time->wday == 0U))
        {
          ret = DC_SRV_RET_ERROR;
        }
        else
        {
          sDate.WeekDay = (uint8_t)time->wday;

          sDate.Date    = (uint8_t)time->mday;
          sDate.Month   = (uint8_t)time->month;
          sDate.Year    = (uint8_t)(time->year - 2000U);
          if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
          {
            dc_time_date.rt_state =  DC_SERVICE_ON;
            ret = (dc_srv_ret_t)dc_com_write(&dc_com_db, DC_COM_TIME_DATE, (void *)&dc_time_date, sizeof(dc_time_date_rt_info_t));
          }
          else
          {
            ret = DC_SRV_RET_ERROR;
          }
        }
      }
    }
    else
    {
      ret = DC_SRV_RET_ERROR;
    }
  }

  return ret;
}

/**
  * @brief  get system date and time
  * @param  dc_time_date_rt_info     (out) date
  * @param  dc_time_data_type_t      (out) time
  * @retval dc_srv_ret_t             return status
  */
dc_srv_ret_t dc_srv_get_time_date(dc_time_date_rt_info_t *dc_time_date_rt_info, dc_time_data_type_t time_date)
{
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;


  /* WARNING : if HAL_RTC_GetDate is called it must be called before HAL_RTC_GetDate */
  HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
  if (time_date != DC_DATE)
  {
    /* Get the RTC current Time */
    dc_time_date_rt_info->hour  = stimestructureget.Hours;
    dc_time_date_rt_info->min   = stimestructureget.Minutes;
    dc_time_date_rt_info->sec   = stimestructureget.Seconds;
    dc_time_date_rt_info->yday  = get_yday(&sdatestructureget);  /* not managed */
    dc_time_date_rt_info->isdst = 0U;  /* not managed */
  }

  /* WARNING : HAL_RTC_GetDate must be called after HAL_RTC_GetTime even if date get is not necessary */
  HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
  if (time_date != DC_TIME)
  {
    /* Get the RTC current Date */
    dc_time_date_rt_info->wday  = sdatestructureget.WeekDay;
    dc_time_date_rt_info->mday  = sdatestructureget.Date;
    dc_time_date_rt_info->month = sdatestructureget.Month;
    dc_time_date_rt_info->year  = sdatestructureget.Year + 2000U;
  }

  return  DC_SRV_RET_OK;
}


void dc_srv_init(void)
{
  DC_COM_TIME_DATE = dc_com_register_serv(&dc_com_db, (void *)&dc_time_date, sizeof(dc_time_date));
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

