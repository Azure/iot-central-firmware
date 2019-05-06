/**
  ******************************************************************************
  * @file    dc_mems.h
  * @author  MCD Application Team
  * @brief   Header for dc_mems.c module
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

#ifndef DC_MEMS_H
#define DC_MEMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dc_control.h"

/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  float pressure;
} dc_pressure_info_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  float humidity;
} dc_humidity_info_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  float temperature;
} dc_temperature_info_t;



typedef struct
{
  int32_t AXIS_X;
  int32_t AXIS_Y;
  int32_t AXIS_Z;
} dc_sensor_axe_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  dc_sensor_axe_t accelerometer;
} dc_accelerometer_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  dc_sensor_axe_t gyroscope;
} dc_gyroscope_info_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  dc_sensor_axe_t magnetometer;
} dc_magnetometer_info_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern osThreadId memsDclibTaskTaskId;
#if (USE_DC_MEMS == 1)
extern dc_com_res_id_t  DC_COM_PRESSURE      ;
extern dc_com_res_id_t  DC_COM_HUMIDITY      ;
extern dc_com_res_id_t  DC_COM_TEMPERATURE   ;
extern dc_com_res_id_t  DC_COM_ACCELEROMETER ;
extern dc_com_res_id_t  DC_COM_GYROSCOPE     ;
extern dc_com_res_id_t  DC_COM_MAGNETOMETER  ;
#endif

/* Exported functions ------------------------------------------------------- */
extern void dc_mems_start(void);
extern void dc_mems_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DC_MEMS_H */

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/
