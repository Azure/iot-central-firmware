/**
  ******************************************************************************
  * @file    dc_control.h
  * @author  MCD Application Team
  * @brief   Header for dc_control.c module
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
#ifndef DC_CONTROL_H
#define DC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "dc_cellular.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  DC_CTRL_OK = 0x00,
  DC_CTRL_ERROR
} dc_ctrl_status_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
} dc_control_button_t;
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern osThreadId dc_CtrlEventTaskId;

extern dc_com_res_id_t    DC_COM_BUTTON_UP    ;
extern dc_com_res_id_t    DC_COM_BUTTON_DN    ;
extern dc_com_res_id_t    DC_COM_BUTTON_RIGHT ;
extern dc_com_res_id_t    DC_COM_BUTTON_LEFT  ;
extern dc_com_res_id_t    DC_COM_BUTTON_SEL   ;


/* Exported functions ------------------------------------------------------------*/

/**
  * @brief  component initialisation
  * @param  none
  * @retval dc_ctrl_status_t     return status
  */
dc_ctrl_status_t dc_ctrl_event_init(void);


/**
  * @brief  component start
  * @param  none
  * @retval dc_ctrl_status_t     return status
  */
dc_ctrl_status_t dc_ctrl_event_start(void);


/**
  * @brief  post an event
  * @param  event_id            event id
  * @retval dc_ctrl_status_t    return status
  */
void dc_ctrl_post_event_normal(dc_com_event_id_t event_id);

/**
  * @brief  debounce event management
  * @param  event_id            event id
  * @retval dc_ctrl_status_t    return status
  */
void dc_ctrl_post_event_debounce(dc_com_event_id_t event_id);

#ifdef __cplusplus
}
#endif

#endif /* __DC_CONTROL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
