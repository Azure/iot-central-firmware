/**
  ******************************************************************************
  * @file    at_datapack.c
  * @author  MCD Application Team
  * @brief   This file provides code for packing/unpacking datas
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
#include "string.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define  DATASTRUCT_POINTER_TYPE   ((uint8_t) 1U)
#define  DATASTRUCT_CONTENT_TYPE   ((uint8_t) 2U)
#define  DATAPACK_HEADER_BYTE_SIZE ((uint8_t) 4U)

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATDATAPACK == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "Datapack:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "Datapack:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "Datapack API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "Datapack ERROR:" format "\n\r", ## args)
#else
#define PrintINFO(format, args...)  printf("Datapack:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("Datapack:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("Datapack API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("Datapack ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATDATAPACK */

/* Private variables ---------------------------------------------------------*/
#if (USE_TRACE_ATDATAPACK == 1U)
static uint32_t datapack_biggest_size = 0U; /* for debug only, used to track maximum struct size */
#endif

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/*
*
* Header
*  - byte0,byte1: message type (provided by user)
*  - byte2,byte3: data size (header not included)
*  - byte5: content type (is data a complete struct data or pointer on struct)
* Data
*
*/
DataPack_Status_t DATAPACK_writePtr(uint8_t *p_buf, uint16_t msgtype, void *p_data)
{
  uint16_t size = sizeof(datapack_structptr_t);
  datapack_structptr_t sptr;
  sptr.structptr = p_data;

  /* write header: message type */
  p_buf[0] = (uint8_t)(msgtype >> 8);
  p_buf[1] = (uint8_t)(msgtype & 0x00FFU);

  /* write header: size */
  p_buf[2] = (uint8_t)(size >> 8);
  p_buf[3] = (uint8_t)(size & 0x00FFU);

  /* write header: content type (pointer of datas) */
  p_buf[4] = DATASTRUCT_POINTER_TYPE;

  /* check pointers */
  if ((p_data == NULL) || (p_buf == NULL))
  {
    PrintErr("DATAPACK_writePtr error (%p, %p)", p_data, p_buf)
    return (DATAPACK_ERROR);
  }

  /* transmit ptr on structure */
  memcpy((void *)&p_buf[DATAPACK_HEADER_BYTE_SIZE + 1U], (void *)&sptr, sizeof(datapack_structptr_t));

  return (DATAPACK_OK);
}

DataPack_Status_t DATAPACK_writeStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data)
{

#if (USE_TRACE_ATDATAPACK == 1U)
  if (size > datapack_biggest_size)
  {
    datapack_biggest_size = size;
  }
  PrintDBG("<MAX SIZE INFO> msgtype=%d size=%d (biggest =%ld)", msgtype, size, datapack_biggest_size)
#endif /* USE_TRACE_ATDATAPACK */

  /* write header: message type */
  p_buf[0] = (uint8_t)(msgtype >> 8);
  p_buf[1] = (uint8_t)(msgtype & 0x00FFU);

  /* write header: size */
  p_buf[2] = (uint8_t)(size >> 8);
  p_buf[3] = (uint8_t)(size & 0x00FFU);

  /* write header: content type (pointer of datas) */
  p_buf[4] = DATASTRUCT_CONTENT_TYPE;

  /* check maximum size and pointer */
  if (((size - DATAPACK_HEADER_BYTE_SIZE) > DATAPACK_MAX_BUF_SIZE) || (p_buf == NULL))
  {
    PrintErr("DATAPACK_writeStruct error")
    return (DATAPACK_ERROR);
  }

  /* transmit structure content */
  memcpy((void *)&p_buf[DATAPACK_HEADER_BYTE_SIZE + 1U],
         (void *)p_data,
         (size_t) size);

  return (DATAPACK_OK);
}

DataPack_Status_t DATAPACK_readPtr(uint8_t *p_buf, uint16_t msgtype, void **p_data)
{
  uint16_t rx_msgtype;
  uint16_t rx_size;
  uint8_t rx_contenttype;

  datapack_structptr_t sptr;

  /* check pointer */
  if (p_buf == NULL)
  {
    PrintErr("DATAPACK_readPtr pointer error (%p)", p_buf)
    return (DATAPACK_ERROR);
  }

  /* check message type, size and content type */
  rx_msgtype = DATAPACK_readMsgType(p_buf);
  if (rx_msgtype != msgtype)
  {
    PrintErr("DATAPACK_readPtr msgtype error (%d, %d)", rx_msgtype, msgtype)
    return (DATAPACK_ERROR);
  }
  rx_size = DATAPACK_readSize(p_buf);
  if (rx_size != sizeof(datapack_structptr_t))
  {
    PrintErr("DATAPACK_readPtr size error (%d, %d)", rx_size, sizeof(datapack_structptr_t))
    return (DATAPACK_ERROR);
  }
  rx_contenttype = p_buf[4];
  if (rx_contenttype != DATASTRUCT_POINTER_TYPE)
  {
    PrintErr("DATAPACK_readPtr content type error")
    return (DATAPACK_ERROR);
  }

  /* retrieve pointer */
  memcpy((void *)&sptr, (void *)&p_buf[DATAPACK_HEADER_BYTE_SIZE + 1U], sizeof(datapack_structptr_t));
  *p_data = sptr.structptr;

  return (DATAPACK_OK);
}

DataPack_Status_t DATAPACK_readStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data)
{
  uint16_t rx_msgtype;
  uint16_t rx_size;
  uint8_t rx_contenttype;

  /* check pointers */
  if ((p_data == NULL) || (p_buf == NULL))
  {
    PrintErr("DATAPACK_readStruct pointer error( %p, %p)", p_data, p_buf)
    return (DATAPACK_ERROR);
  }

  /* check message type, size and content type */
  rx_msgtype = DATAPACK_readMsgType(p_buf);
  if (rx_msgtype != msgtype)
  {
    PrintErr("DATAPACK_readStruct msgtype error (%d, %d)", rx_msgtype, msgtype)
    return (DATAPACK_ERROR);
  }
  rx_size = DATAPACK_readSize(p_buf);
  if (rx_size != size)
  {
    PrintErr("DATAPACK_readStruct size error (%d, %d)", rx_size, size)
    return (DATAPACK_ERROR);
  }
  rx_contenttype = p_buf[4];
  if (rx_contenttype != DATASTRUCT_CONTENT_TYPE)
  {
    PrintErr("DATAPACK_readStruct content type error")
    return (DATAPACK_ERROR);
  }

  /* retrieve data */
  memcpy((void *)p_data,
         (void *)&p_buf[DATAPACK_HEADER_BYTE_SIZE + 1U],
         (size_t) size);

  return (DATAPACK_OK);
}


uint16_t DATAPACK_readMsgType(uint8_t *p_buf)
{
  uint16_t msgtype;
  /* read header: message type */
  msgtype = ((uint16_t)p_buf[0] << 8) + (uint16_t)p_buf[1];
  return ((uint16_t)msgtype);
}

uint16_t DATAPACK_readSize(uint8_t *p_buf)
{
  uint16_t size;
  /* read header: size */
  size = ((uint16_t)p_buf[2] << 8) + (uint16_t)p_buf[3];
  return (size);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

