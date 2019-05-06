/**
  ******************************************************************************
  * @file    trace_interface.c
  * @author  MCD Application Team
  * @brief   This file contains trace define utilities
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
#include "trace_interface.h"
#include "main.h"


/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define MAX_HEX_PRINT_SIZE 210U

/* Private variables ---------------------------------------------------------*/
#if (((TRACE_IF_TRACES_ITM == 1U) && (TRACE_IF_TRACES_UART == 1U)) || (TRACE_IF_TRACES_ITM == 1U) || (TRACE_IF_TRACES_UART == 1U))
static  uint8_t car[MAX_HEX_PRINT_SIZE];
#else
uint8_t car[MAX_HEX_PRINT_SIZE];
#endif

/* Private function prototypes -----------------------------------------------*/
static void ITM_Out(uint32_t port, uint32_t ch);

/* Global variables ----------------------------------------------------------*/
uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];

/* Functions Definition ------------------------------------------------------*/
/* static functions */
static void ITM_Out(uint32_t port, uint32_t ch)
{
  /* check port validity (0-31)*/
  if (port <= 31U)
  {
    uint32_t tmp_mask;
    tmp_mask = (ITM->TER & (1UL << port));
    if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&   /* ITM enabled ? */
        (tmp_mask != 0UL))   /* ITM selected Port enabled ? */
    {

      /* Wait until ITM port is ready */
      while (ITM->PORT[port].u32 == 0UL)
      {
        __NOP();
      }

      /* then send data, one byte at a time */
      ITM->PORT[port].u8 = (uint8_t) ch;
    }
  }
}

/* exported functions */
void traceIF_itmPrint(uint8_t port, uint8_t *ptr, uint16_t len)
{
  uint32_t i;
  for (i = 0U; i < len; i++)
  {
    ITM_Out((uint32_t) port, (uint32_t) *ptr);
    ptr++;
  }
}

void traceIF_uartPrint(uint8_t port, uint8_t *ptr, uint16_t len)
{
  uint32_t i;
  for (i = 0U; i < len; i++)
  {
    HAL_UART_Transmit(&TRACE_INTERFACE_UART_HANDLE, (uint8_t *)ptr, 1U, 0xFFFFU);
    ptr++;
  }
}


void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len)
{
  uint32_t i;

  if ((len * 2U + 1U) > MAX_HEX_PRINT_SIZE)
  {
    TracePrint(chan,  level, "TR (%d/%d)\n\r", (MAX_HEX_PRINT_SIZE >> 1) - 2U, len)
    len = (MAX_HEX_PRINT_SIZE >> 1) - 2U;
  }

  for (i = 0U; i < len; i++)
  {
    uint8_t digit = ((buff[i] >> 4U) & 0xfU);
    if (digit <= 9U)
    {
      car[2U * i] =  digit + 0x30U;
    }
    else
    {
      car[2U * i] =  digit + 0x41U - 10U;
    }

    digit = (0xfU & buff[i]);
    if (digit <= 9U)
    {
      car[2U * i + 1U] =  digit + 0x30U;
    }
    else
    {
      car[2U * i + 1U] =  digit + 0x41U - 10U;
    }
  }
  car[2U * i] =  0U;

  TracePrint(chan,  level, "%s ", (char *)car)
}

void traceIF_BufCharPrint(dbg_channels_t chan, dbg_levels_t level, char *buf, uint16_t size)
{
  for (uint32_t cpt = 0U; cpt < size; cpt++)
  {
    if (buf[cpt] == 0U)
    {
      TracePrint(chan, level, "<NULL CHAR>")
    }
    else if (buf[cpt] == '\r')
    {
      TracePrint(chan, level, "<CR>")
    }
    else if (buf[cpt] == '\n')
    {
      TracePrint(chan, level, "<LF>")
    }
    else if (buf[cpt] == 0x1AU)
    {
      TracePrint(chan, level, "<CTRL-Z>")
    }
    else if ((buf[cpt] >= 0x20U) && (buf[cpt] <= 0x7EU))
    {
      /* printable char */
      TracePrint(chan, level, "%c", buf[cpt])
    }
    else
    {
      /* Special Character - not printable */
      TracePrint(chan, level, "<SC>")
    }
  }
  TracePrint(chan, level, "\n\r")
}

void traceIF_BufHexPrint(dbg_channels_t chan, dbg_levels_t level, char *buf, uint16_t size)
{
  for (uint32_t cpt = 0U; cpt < size; cpt++)
  {
    TracePrint(chan, level, "0x%02x ", (uint8_t) buf[cpt])
    if ((cpt != 0U) && ((cpt + 1U) % 16U == 0U))
    {
      TracePrint(chan, level, "\n\r")
    }
  }
  TracePrint(chan, level, "\n\r")
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
