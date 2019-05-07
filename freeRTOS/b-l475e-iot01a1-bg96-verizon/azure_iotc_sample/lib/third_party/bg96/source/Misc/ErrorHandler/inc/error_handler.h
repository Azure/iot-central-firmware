/**
  ******************************************************************************
  * @file    error_handler.h
  * @author  MCD Application Team
  * @brief   Header for error_handler.c module
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
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "trace_interface.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  ERROR_NO = 0,
  ERROR_DEBUG,
  ERROR_WARNING,
  ERROR_FATAL,
} error_gravity_t;

typedef struct
{
  dbg_channels_t  channel; /* channel where error occured */
  int32_t             errorId; /* number identifying the error in the channel */
  error_gravity_t gravity; /* error gravity */
  uint32_t        count;   /* count how many errors have been logged since the beginning */

} error_handler_decript_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void ERROR_Handler_Init(void);
void ERROR_Handler(dbg_channels_t chan, int32_t errorId, error_gravity_t gravity);
void ERROR_Dump_All(void);
void ERROR_Dump_Last(void);


#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
