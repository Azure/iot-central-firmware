/**
  ******************************************************************************
  * @file    board_buttons.c
  * @author  MCD Application Team
  * @brief   Implements functions for user buttons actions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "board_buttons.h"
#include "dc_common.h"
#include "dc_control.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

void BB_UserButton_Pressed(void)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  dc_ctrl_post_event_debounce(DC_COM_BUTTON_DN);
  /* change event above if needed:
     DC_COM_BUTTON_UP   : used by HTTP CLIENT application to active/deactivate
     DC_COM_BUTTON_DN   : used by PING application to start a ping
     DC_COM_BUTTON_RIGHT: not used
     DC_COM_BUTTON_LEFT : not used
     DC_COM_BUTTON_SEL  : not used
   Note: current implementation is a temporary solution...
  */

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
