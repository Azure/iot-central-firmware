/**
  ******************************************************************************
  * @file    trace_interface.h
  * @author  MCD Application Team
  * @brief   Header for trace_interface.c module
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
#ifndef TRACE_INTERFACE_H
#define TRACE_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "stm32l4xx_hal.h"
#include <string.h>

/* define debug levels (bitmap) */
typedef enum
{
  DBL_LVL_P0    = 0x01U,
  DBL_LVL_P1    = 0x02U,
  DBL_LVL_P2    = 0x04U,
  DBL_LVL_WARN  = 0x08U,
  DBL_LVL_ERR   = 0x10U,
} dbg_levels_t;

/* following flags select debug interface(s) to use : to be defined in plf_sw_config.h
  #define TRACE_IF_TRACES_ITM     (1)
  #define TRACE_IF_TRACES_UART    (1)
*/

/* DEBUG MASK defines the allowed traces : to be defined in plf_sw_config.h */
/* Full traces */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_P1 | DBL_LVL_P2 | DBL_LVL_WARN | DBL_LVL_ERR) */
/* Warn and Error traces only */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_WARN | DBL_LVL_ERR) */


/* Maximum buffer size (per channel) */
#define DBG_IF_MAX_BUFFER_SIZE  (uint16_t)(256)

/* Define here the list of 32 ITM channels (0 to 31) */
typedef enum
{
  DBG_CHAN_GENERIC = 0,
  DBG_CHAN_MAIN,
  DBG_CHAN_ATCMD,
  DBG_CHAN_COM,
  DBG_CHAN_HTTP,
  DBG_CHAN_PING,
  DBG_CHAN_IPC,
  DBG_CHAN_PPPOSIF,
  DBG_CHAN_CELLULAR_SERVICE,
  DBG_CHAN_NIFMAN,
  DBG_CHAN_DATA_CACHE,
  DBG_CHAN_UTILITIES,
  DBG_CHAN_ERROR_LOGGER,
  DBG_CHAN_MAX_VALUE        /* keep last */
} dbg_channels_t;

extern uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];

void traceIF_itmPrint(uint8_t port, uint8_t *ptr, uint16_t len);
void traceIF_uartPrint(uint8_t port, uint8_t *ptr, uint16_t len);
void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len);

void traceIF_BufCharPrint(dbg_channels_t chan, dbg_levels_t level, char *buf, uint16_t size);
void traceIF_BufHexPrint(dbg_channels_t chan, dbg_levels_t level, char *buf, uint16_t size);

#if ((TRACE_IF_TRACES_ITM == 1U) && (TRACE_IF_TRACES_UART == 1U))
#define TracePrint(chan, lvl, format, args...) if (((lvl) & TRACE_IF_MASK) != 0U)  {\
                                            sprintf((char*)dbgIF_buf[(chan)], format "", ## args);\
                                            traceIF_itmPrint((uint8_t)(chan), (uint8_t *)dbgIF_buf[(chan)],\
                                                (uint16_t)strlen((char *)dbgIF_buf[(chan)]));\
                                            traceIF_uartPrint((uint8_t)(chan), (uint8_t *)dbgIF_buf[(chan)],\
                                                (uint16_t)strlen((char *)dbgIF_buf[(chan)]));}
#elif (TRACE_IF_TRACES_ITM == 1U)
#define TracePrint(chan, lvl, format, args...) if (((lvl) & TRACE_IF_MASK) != 0U)   {\
                                            sprintf((char*)dbgIF_buf[(chan)], format "", ## args);\
                                            traceIF_itmPrint((uint8_t)(chan), (uint8_t *)dbgIF_buf[(chan)],\
                                                (uint16_t)strlen((char *)dbgIF_buf[(chan)]));}
#elif (TRACE_IF_TRACES_UART == 1U)
#define TracePrint(chan, lvl, format, args...) if (((lvl) & TRACE_IF_MASK) != 0U)  {\
                                              sprintf((char*)dbgIF_buf[(chan)], format "", ## args);\
                                              traceIF_uartPrint((uint8_t)(chan), (uint8_t *)dbgIF_buf[(chan)],\
                                                  (uint16_t)strlen((char *)dbgIF_buf[(chan)]));}
#else
#define TracePrint(chan, lvl, format, args...) do {} while(0);
#endif

#define TracePrintBufChar(chan, lvl, pbuf, size) traceIF_BufCharPrint((chan), (lvl), (pbuf), (size))
#define TracePrintBufHex(chan, lvl, pbuf, size)  traceIF_BufHexPrint((chan), (lvl), (pbuf), (size))


#ifdef __cplusplus
}
#endif

#endif /* TRACE_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
