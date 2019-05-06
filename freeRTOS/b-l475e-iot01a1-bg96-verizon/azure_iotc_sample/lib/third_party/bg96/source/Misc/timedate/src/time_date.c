/**
  ******************************************************************************
  * @file    time_date.c
  * @author  MCD Application Team
  * @brief   This file contains date and time utilities
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
#include "time_date.h"
#include "error_handler.h"
#include "dc_time.h"
#include "menu_utils.h"



/* Private macros ------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define TIMEDATE_SETUP_LABEL         "Time Date Menu"
#define TIMEDATE_DEFAULT_VALUE       "Mon, 1 Jan 2018 00:00:00"
#define TIMEDATE_DEFAULT_PARAMA_NB   1
#define TIMEDATE_STRING_SIZE         30U


/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static uint8_t timedate_input_sring[TIMEDATE_STRING_SIZE];
static uint8_t *timedate_day_of_week_sring[8] =
{
  (uint8_t *)"",
  (uint8_t *)"Mon",
  (uint8_t *)"Tue",
  (uint8_t *)"Wed",
  (uint8_t *)"Thu",
  (uint8_t *)"Fri",
  (uint8_t *)"Sat",
  (uint8_t *)"Sun",
};

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

int32_t timedate_http_date_set(char *dateStr)
{
  dc_time_date_rt_info_t  dc_time_date_rt_info;
  uint8_t  dow[8], month[4];
  int32_t day, year, hour, min, sec;
  int32_t ret = -1;

  memset(dow, 0, sizeof(dow));
  memset(month, 0, sizeof(month));
  day = year = hour = min = sec = 0;

  int32_t count = sscanf(dateStr, "HTTP/1.1 200 OK\r\nDate: %s %ld %s %ld %02ld:%02ld:%02ld ", dow, &day, month, &year, &hour, &min, &sec);
  if (count >= 7)
  {

    if (strcmp((const char *)dow, "Mon,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_MONDAY;
    }
    else if (strcmp((const char *)dow, "Tue,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_TUESDAY;
    }
    else if (strcmp((const char *)dow, "Wed,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_WEDNESDAY;
    }
    else if (strcmp((const char *)dow, "Thu,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_THURSDAY;
    }
    else if (strcmp((const char *)dow, "Fri,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_FRIDAY;
    }
    else if (strcmp((const char *)dow, "Sat,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_SATURDAY;
    }
    else if (strcmp((const char *)dow, "Sun,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_SUNDAY;
    }
    else {}


    if (strcmp((const char *)month, "Jan") == 0)
    {
      dc_time_date_rt_info.month = 1U;
    }
    else if (strcmp((const char *)month, "Feb") == 0)
    {
      dc_time_date_rt_info.month = 2U;
    }
    else if (strcmp((const char *)month, "Mar") == 0)
    {
      dc_time_date_rt_info.month = 3U;
    }
    else if (strcmp((const char *)month, "Apr") == 0)
    {
      dc_time_date_rt_info.month = 4U;
    }
    else if (strcmp((const char *)month, "May") == 0)
    {
      dc_time_date_rt_info.month = 5U;
    }
    else if (strcmp((const char *)month, "Jun") == 0)
    {
      dc_time_date_rt_info.month = 6U;
    }
    else if (strcmp((const char *)month, "Jul") == 0)
    {
      dc_time_date_rt_info.month = 7U;
    }
    else if (strcmp((const char *)month, "Aug") == 0)
    {
      dc_time_date_rt_info.month = 8U;
    }
    else if (strcmp((const char *)month, "Sep") == 0)
    {
      dc_time_date_rt_info.month = 9U;
    }
    else if (strcmp((const char *)month, "Oct") == 0)
    {
      dc_time_date_rt_info.month = 10U;
    }
    else if (strcmp((const char *)month, "Nov") == 0)
    {
      dc_time_date_rt_info.month = 11U;
    }
    else if (strcmp((const char *)month, "Dec") == 0)
    {
      dc_time_date_rt_info.month = 12U;
    }
    else { }

    dc_time_date_rt_info.mday = (uint32_t)day;
    dc_time_date_rt_info.year = (uint32_t)year;

    dc_time_date_rt_info.hour = (uint32_t)hour;
    dc_time_date_rt_info.min  = (uint32_t)min;
    dc_time_date_rt_info.sec  = (uint32_t)sec;
    dc_srv_set_time_date(&dc_time_date_rt_info, DC_DATE_AND_TIME);
    ret = 0;
  }
  return ret;
}

int32_t timedate_set(char *dateStr)
{
  dc_time_date_rt_info_t  dc_time_date_rt_info;
  uint8_t dow[8], month[4];
  int32_t day, year, hour, min, sec;
  int32_t ret = -1;
  memset(dow, 0, sizeof(dow));
  memset(month, 0, sizeof(month));
  day = year = hour = min = sec = 0;

  int32_t count = sscanf(dateStr, "%s %ld %s %ld %02ld:%02ld:%02ld ", dow, &day, month, &year, &hour, &min, &sec);
  if (count >= 7)
  {
    /*     PrintINFO("Configuring date from %s\n\r", dateStr); */

    if (strcmp((const char *)dow, "Mon,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_MONDAY;
    }
    else if (strcmp((const char *)dow, "Tue,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_TUESDAY;
    }
    else if (strcmp((const char *)dow, "Wed,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_WEDNESDAY;
    }
    else if (strcmp((const char *)dow, "Thu,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_THURSDAY;
    }
    else if (strcmp((const char *)dow, "Fri,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_FRIDAY;
    }
    else if (strcmp((const char *)dow, "Sat,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_SATURDAY;
    }
    else if (strcmp((const char *)dow, "Sun,") == 0)
    {
      dc_time_date_rt_info.wday = RTC_WEEKDAY_SUNDAY;
    }
    else {}

    if (strcmp((const char *)month, "Jan") == 0)
    {
      dc_time_date_rt_info.month = 1U;
    }
    else if (strcmp((const char *)month, "Feb") == 0)
    {
      dc_time_date_rt_info.month = 2U;
    }
    else if (strcmp((const char *)month, "Mar") == 0)
    {
      dc_time_date_rt_info.month = 3U;
    }
    else if (strcmp((const char *)month, "Apr") == 0)
    {
      dc_time_date_rt_info.month = 4U;
    }
    else if (strcmp((const char *)month, "May") == 0)
    {
      dc_time_date_rt_info.month = 5U;
    }
    else if (strcmp((const char *)month, "Jun") == 0)
    {
      dc_time_date_rt_info.month = 6U;
    }
    else if (strcmp((const char *)month, "Jul") == 0)
    {
      dc_time_date_rt_info.month = 7U;
    }
    else if (strcmp((const char *)month, "Aug") == 0)
    {
      dc_time_date_rt_info.month = 8U;
    }
    else if (strcmp((const char *)month, "Sep") == 0)
    {
      dc_time_date_rt_info.month = 9U;
    }
    else if (strcmp((const char *)month, "Oct") == 0)
    {
      dc_time_date_rt_info.month = 10U;
    }
    else if (strcmp((const char *)month, "Nov") == 0)
    {
      dc_time_date_rt_info.month = 11U;
    }
    else if (strcmp((const char *)month, "Dec") == 0)
    {
      dc_time_date_rt_info.month = 12U;
    }
    else {}

    dc_time_date_rt_info.mday = (uint32_t)day;
    dc_time_date_rt_info.year = (uint32_t)year;

    dc_time_date_rt_info.hour = (uint32_t)hour;
    dc_time_date_rt_info.min  = (uint32_t)min;
    dc_time_date_rt_info.sec  = (uint32_t)sec;
    dc_srv_set_time_date(&dc_time_date_rt_info, DC_DATE_AND_TIME);
    ret = 0;
  }

  return ret;
}


void timedate_setup_handler(void)
{
  PrintSetup("timedate_setup_handler\n\r");

  menu_utils_get_string((MU_CHAR_t *)"Date GMT <Day, dd Month yyyy hh:mm:ss> (ex: Mon, 11 Dec 2017 17:22:05) ", timedate_input_sring, TIMEDATE_STRING_SIZE);
  timedate_set((char *)timedate_input_sring);

}

void timedate_setup_dump(void)
{
  dc_time_date_rt_info_t  dc_time_date_rt_info;
  dc_srv_get_time_date(&dc_time_date_rt_info, DC_DATE_AND_TIME);

  PrintSetup("Date: %s %02ld/%02ld/%ld - %02ld:%02ld:%02ld\n\r",
             timedate_day_of_week_sring[dc_time_date_rt_info.wday],
             dc_time_date_rt_info.mday,
             dc_time_date_rt_info.month,
             dc_time_date_rt_info.year,
             dc_time_date_rt_info.hour,
             dc_time_date_rt_info.min,
             dc_time_date_rt_info.sec);

}


int32_t timedate_init(void)
{
  /*   return setup_record(SETUP_APPLI_TIMEDATE, timedate_setup_handler, timedate_setup_dump, timedate_default_setup_table, TIMEDATE_DEFAULT_PARAMA_NB); */
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
