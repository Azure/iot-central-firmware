/**
  ******************************************************************************
  * @file    error_handler.c
  * @author  MCD Application Team
  * @brief   This file contains error utilities
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
#include "error_handler.h"
#include "plf_config.h"



/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ERROR_HANDLER == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ERROR_LOGGER, DBL_LVL_P0, "" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ERROR_LOGGER, DBL_LVL_1, "" format "\n\r", ## args)
#else
#define PrintINFO(format, args...)  printf("ATCore:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("ATCore:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#endif /* USE_TRACE_ERROR_HANDLER */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define MAX_ERROR_ENTRIES (32U)     /* log only last MAX_ERROR_ENTRIES erros */
#define MAX_ERROR_COUNTER (0xFFFFU) /* count how many errors have been logged since the beginning */

/* Private variables ---------------------------------------------------------*/
static error_handler_decript_t errors_table[MAX_ERROR_ENTRIES]; /* error array */
static uint16_t error_index   = 0U; /* current error index */
static uint16_t error_counter = 0U; /* total number of errors */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
void ERROR_Handler_Init(void)
{
  uint32_t i;

  /* initialize error array */
  for (i = 0U; i < MAX_ERROR_ENTRIES; i++)
  {
    errors_table[i].channel = DBG_CHAN_ERROR_LOGGER; /* default value = self (ie no error) */
    errors_table[i].errorId = 0;
    errors_table[i].gravity = ERROR_NO;
  }

}

void ERROR_Handler(dbg_channels_t chan, int32_t errorId, error_gravity_t gravity)
{
  /* if this is the very first error, init error array */
  if (error_counter == 0U)
  {
    ERROR_Handler_Init();
  }

  /* log the error */
  error_counter = (error_counter + 1U) % MAX_ERROR_COUNTER;
  errors_table[error_index].count = error_counter;
  errors_table[error_index].channel = chan;
  errors_table[error_index].errorId = errorId;
  errors_table[error_index].gravity = gravity;

  PrintINFO("LOG ERROR #%d: channel=%d / errorId=%d / gravity=%d", error_counter, chan, errorId, gravity)

  /* endless loop if error is fatal */
  if (gravity == ERROR_FATAL)
  {
    HAL_Delay(1000U);
    NVIC_SystemReset();
    while (1)
    {
    }
  }

  /* increment error index */
  error_index = (error_index + 1U) %  MAX_ERROR_ENTRIES;
}

void ERROR_Dump_All(void)
{
  uint32_t i;
  if (error_counter)
  {
    /* Dump errors array */
    for (i = 0U; i < MAX_ERROR_ENTRIES; i++)
    {
      if (errors_table[i].gravity != ERROR_NO)
      {
        PrintINFO("DUMP ERROR[%d] (#%d): channel=%d / errorId=%d / gravity=%d",
                  i,
                  errors_table[i].count,
                  errors_table[i].channel,
                  errors_table[i].errorId,
                  errors_table[i].gravity)
      }
    }
  }
}

void ERROR_Dump_Last(void)
{
#if ((USE_TRACE_ERROR_HANDLER == 1U) || (USE_PRINTF == 1U))
  if (error_counter)
  {
    /* get last error index */
    uint32_t previous_index = (error_index + (MAX_ERROR_ENTRIES - 1)) % MAX_ERROR_ENTRIES;

    PrintINFO("DUMP LAST ERROR[%d] (#%d): channel=%d / errorId=%d / gravity=%d",
              previous_index,
              errors_table[previous_index].count,
              errors_table[previous_index].channel,
              errors_table[previous_index].errorId,
              errors_table[previous_index].gravity)
  }
#endif
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
