/**
  ******************************************************************************
  * @file    cellular_init.h
  * @author  MCD Application Team
  * @brief   Header for cellular_init.c module
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
#ifndef CELLULAR_INIT_H
#define CELLULAR_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dc_common.h"
#include "dc_cellular.h"
#include "com_sockets.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void cellular_init(void);
void cellular_start(void);
void cellular_modem_start(void);



#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_INIT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
