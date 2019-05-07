/**
  ******************************************************************************
  * @file    ipc_rxfifo.c
  * @author  MCD Application Team
  * @brief   This file provides common code for managing IPC RX FIFO
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
#include "ipc_rxfifo.h"
#include "ipc_common.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_IPC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_IPC, DBL_LVL_P0, "IPC:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_IPC, DBL_LVL_P1, "IPC:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_IPC, DBL_LVL_ERR, "IPC ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P0, (char *)pbuf, size);
#define PrintBufHexa(pbuf, size)   TracePrintBufHex(DBG_CHAN_ATCMD, DBL_LVL_P0, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("IPC:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("IPC:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("IPC ERROR:" format "\n\r", ## args);
#define PrintBuf(pbuf, size)        do {} while(0);
#define PrintBufHexa(pbuf, size)    do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(pbuf, size)        do {} while(0);
#define PrintBufHexa(pbuf, size)    do {} while(0);
#endif /* USE_TRACE_IPC */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void RXFIFO_incrementTail(IPC_Handle_t *hipc, uint16_t inc_size);
static void RXFIFO_incrementHead(IPC_Handle_t *hipc);
static void RXFIFO_readMsgHeader(IPC_Handle_t *hipc, IPC_RxHeader_t *header);
static void RXFIFO_updateMsgHeader(IPC_Handle_t *hipc);
static void RXFIFO_prepareNextMsgHeader(IPC_Handle_t *hipc);
static void rearm_RX_IT(IPC_Handle_t *hipc);

/* Functions Definition ------------------------------------------------------*/
void RXFIFO_init(IPC_Handle_t *hipc)
{
  memset(hipc->RxQueue.data, 0, sizeof(uint8_t) * IPC_RXBUF_MAXSIZE);
  hipc->RxQueue.index_read = 0U;
  hipc->RxQueue.index_write = IPC_RXMSG_HEADER_SIZE;
  hipc->RxQueue.current_msg_index = 0U;
  hipc->RxQueue.current_msg_size = 0U;
  hipc->RxQueue.nb_unread_msg = 0U;

#if (DBG_IPC_RX_FIFO != 0U)
  /* init debug infos */
  hipc->dbgRxQueue.total_RXchar = 0U;
  hipc->dbgRxQueue.cpt_RXPause = 0U;
  hipc->dbgRxQueue.free_bytes = IPC_RXBUF_MAXSIZE;
  hipc->dbgRxQueue.queue_pos = 0U;
  hipc->dbgRxQueue.msg_info_queue[0].start_pos = hipc->RxQueue.index_read;
  hipc->dbgRxQueue.msg_info_queue[0].size = 0U;
  hipc->dbgRxQueue.msg_info_queue[0].complete = 0U;
#endif /* DBG_IPC_RX_FIFO */
}

void RXFIFO_writeCharacter(IPC_Handle_t *hipc, uint8_t rxChar)
{
  hipc->RxQueue.data[hipc->RxQueue.index_write] = rxChar;

  hipc->RxQueue.current_msg_size++;

#if (DBG_IPC_RX_FIFO != 0U)
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].size = hipc->RxQueue.current_msg_size;
#endif /* DBG_IPC_RX_FIFO */

  RXFIFO_incrementHead(hipc);

  if (hipc->State != IPC_STATE_PAUSED)
  {
    /* rearm RX Interrupt */
    rearm_RX_IT(hipc);
  }

  /* check if the char received is an end of message */
  if ((*hipc->CheckEndOfMsgCallback)(rxChar) == 1U)
  {
    hipc->RxQueue.nb_unread_msg++;

    /* update header for message received */
    RXFIFO_updateMsgHeader(hipc);

    /* save start position of next message */
    hipc->RxQueue.current_msg_index = hipc->RxQueue.index_write;

    /* reset current msg size */
    hipc->RxQueue.current_msg_size = 0U;

    /* reserve place for next msg header */
    RXFIFO_prepareNextMsgHeader(hipc);

    /* msg received: call client callback */
    (* hipc->RxClientCallback)((void *)hipc);
  }

  return;
}

int16_t RXFIFO_read(IPC_Handle_t *hipc, IPC_RxMessage_t *o_Msg)
{
  IPC_RxHeader_t header;
  uint16_t oversize = 0U;

#if (DBG_IPC_RX_FIFO != 0U)
  PrintDBG(" *** start pos=%d ", hipc->RxQueue.index_read)
#endif /* DBG_IPC_RX_FIFO */

  /* read message header */
  RXFIFO_readMsgHeader(hipc, &header);

  if (header.complete != 1U)
  {
    /* error: trying to read an uncomplete message */
    return (-1);
  }

  /* jump header */
  RXFIFO_incrementTail(hipc, IPC_RXMSG_HEADER_SIZE);

#if (DBG_IPC_RX_FIFO != 0U)
  PrintDBG(" *** data pos=%d ", hipc->RxQueue.index_read)
  PrintDBG(" *** size=%d ", header.size)
  PrintDBG(" *** free bytes before read=%d ", hipc->dbgRxQueue.free_bytes)
#endif /* DBG_IPC_RX_FIFO */

  /* update size in output structure */
  o_Msg->size = header.size;

  /* copy msg content to output structure */
  if ((hipc->RxQueue.index_read + header.size) > IPC_RXBUF_MAXSIZE)
  {
    /* message is split in 2 parts in the circular buffer */
    oversize = (hipc->RxQueue.index_read + header.size - IPC_RXBUF_MAXSIZE);
    uint16_t remaining_size = header.size - oversize;
    memcpy((void *) & (o_Msg->buffer[0]),
           (void *) & (hipc->RxQueue.data[hipc->RxQueue.index_read]),
           (size_t) remaining_size);
    memcpy((void *) & (o_Msg->buffer[header.size - oversize]),
           (void *) & (hipc->RxQueue.data[0]),
           (size_t) oversize);

#if (DBG_IPC_RX_FIFO != 0U)
    PrintDBG("override end of buffer")
#endif /* DBG_IPC_RX_FIFO */
  }
  else
  {
    /* message is contiguous in the circular buffer */
    memcpy((void *)o_Msg->buffer, (void *) & (hipc->RxQueue.data[hipc->RxQueue.index_read]), (size_t) header.size);
  }

  /* increment tail index to the next message */
  RXFIFO_incrementTail(hipc, header.size);

#if (DBG_IPC_RX_FIFO != 0U)
  /* update free_bytes infos */
  hipc->dbgRxQueue.free_bytes = RXFIFO_getFreeBytes(hipc);
  PrintDBG(" *** free after read bytes=%d ", hipc->dbgRxQueue.free_bytes)
#endif /* DBG_IPC_RX_FIFO */

  /* msg has been read */
  hipc->RxQueue.nb_unread_msg--;

  return ((int16_t)hipc->RxQueue.nb_unread_msg);
}

#if (IPC_USE_STREAM_MODE == 1U)
void RXFIFO_stream_init(IPC_Handle_t *hipc)
{
  memset(hipc->RxBuffer.data, 0,  sizeof(uint8_t) * IPC_RXBUF_STREAM_MAXSIZE);
  hipc->RxBuffer.index_read = 0U;
  hipc->RxBuffer.index_write = 0U;
  hipc->RxBuffer.available_char = 0U;
  hipc->RxBuffer.total_rcv_count = 0U;
}

void RXFIFO_writeStream(IPC_Handle_t *hipc, uint8_t rxChar)
{
  hipc->RxBuffer.data[hipc->RxBuffer.index_write] = rxChar;

  /* rearm RX Interrupt */
  rearm_RX_IT(hipc);

  hipc->RxBuffer.index_write++;
  hipc->RxBuffer.total_rcv_count++;

  if (hipc->RxBuffer.index_write >= IPC_RXBUF_STREAM_MAXSIZE)
  {
    hipc->RxBuffer.index_write = 0;
  }
  hipc->RxBuffer.available_char++;

  (* hipc->RxClientCallback)((void *)hipc);

}
#endif /* IPC_USE_STREAM_MODE */

void RXFIFO_print_data(IPC_Handle_t *hipc, uint16_t index, uint16_t size, uint8_t readable)
{
  PrintINFO("DUMP RX QUEUE: (index=%d) (size=%d)", index, size)
  if ((index + size) > IPC_RXBUF_MAXSIZE)
  {
    /* in case buffer loops back to index 0 */
    /* print first buffer part (until end of queue) */
    PrintBuf((uint8_t *)&hipc->RxQueue.data[index], (IPC_RXBUF_MAXSIZE - index))
    /* print second buffer part */
    PrintBuf((uint8_t *)&hipc->RxQueue.data[0], (size - (IPC_RXBUF_MAXSIZE - index)))

    PrintINFO("dump same in hexa:")
    /* print first buffer part (until end of queue) */
    PrintBufHexa((uint8_t *)&hipc->RxQueue.data[index], (IPC_RXBUF_MAXSIZE - index))
    /* print second buffer part */
    PrintBufHexa((uint8_t *)&hipc->RxQueue.data[0], (size - (IPC_RXBUF_MAXSIZE - index)))
  }
  else
  {
    PrintBuf((uint8_t *)&hipc->RxQueue.data[index], size)
    PrintINFO("dump same in hexa:")
    PrintBufHexa((uint8_t *)&hipc->RxQueue.data[index], size)
  }
  PrintINFO("\r\n")
}

void RXFIFO_readMsgHeader_at_pos(IPC_Handle_t *hipc, IPC_RxHeader_t *header, uint16_t pos)
{
  uint8_t header_byte1, header_byte2;
  uint16_t index;

#if (DBG_IPC_RX_FIFO != 0U)
  PrintDBG("DBG RXFIFO_readMsgHeader: index_read = %d", hipc->RxQueue.index_read)
#endif /* DBG_IPC_RX_FIFO */

  /* read header bytes */
  index =  pos; /* hipc->RxQueue.index_read; */
  header_byte1 = hipc->RxQueue.data[index];
  index = (index + 1U) % IPC_RXBUF_MAXSIZE;
  header_byte2 = hipc->RxQueue.data[index];

  PrintDBG("header_byte1[0x%x] header_byte2[0x%x]", header_byte1, header_byte2)

  /* get msg complete bit */
  header->complete = (IPC_RXMSG_HEADER_COMPLETE_MASK & header_byte1) >> 7;
  /* get msg size */
  header->size = (((uint16_t)IPC_RXMSG_HEADER_SIZE_MASK & (uint16_t)header_byte1) << 8);
  header->size = header->size + header_byte2;

  PrintDBG("complete=%d size=%d", header->complete, header->size)

  return;
}

uint16_t RXFIFO_getFreeBytes(IPC_Handle_t *hipc)
{
  uint16_t free_bytes;

  if (hipc->RxQueue.index_write > hipc->RxQueue.index_read)
  {
    free_bytes = (IPC_RXBUF_MAXSIZE - hipc->RxQueue.index_write +  hipc->RxQueue.index_read);
  }
  else
  {
    free_bytes =  hipc->RxQueue.index_read - hipc->RxQueue.index_write;
  }

  return (free_bytes);
}

/* Private function Definition -----------------------------------------------*/
static void RXFIFO_incrementTail(IPC_Handle_t *hipc, uint16_t inc_size)
{
  hipc->RxQueue.index_read = (hipc->RxQueue.index_read + inc_size) % IPC_RXBUF_MAXSIZE;
}

static void RXFIFO_incrementHead(IPC_Handle_t *hipc)
{
  uint16_t free_bytes = 0U;

  hipc->RxQueue.index_write = (hipc->RxQueue.index_write + 1U) % IPC_RXBUF_MAXSIZE;

  free_bytes = RXFIFO_getFreeBytes(hipc);

#if (DBG_IPC_RX_FIFO != 0U)
  hipc->dbgRxQueue.free_bytes = free_bytes;
#endif /* DBG_IPC_RX_FIFO */

  if (free_bytes <= IPC_RXBUF_THRESHOLD)
  {
    hipc->State = IPC_STATE_PAUSED;

#if (DBG_IPC_RX_FIFO != 0U)
    hipc->dbgRxQueue.cpt_RXPause++;
#endif /* DBG_IPC_RX_FIFO */
  }
}

static void RXFIFO_readMsgHeader(IPC_Handle_t *hipc, IPC_RxHeader_t *header)
{
  uint8_t header_byte1, header_byte2;
  uint16_t index;

#if (DBG_IPC_RX_FIFO != 0U)
  PrintDBG("DBG RXFIFO_readMsgHeader: index_read = %d", hipc->RxQueue.index_read)
#endif /* DBG_IPC_RX_FIFO */

  /* read header bytes */
  index =  hipc->RxQueue.index_read;
  header_byte1 = hipc->RxQueue.data[index];
  index = (index + 1U) % IPC_RXBUF_MAXSIZE;
  header_byte2 = hipc->RxQueue.data[index];

  /* get msg complete bit */
  header->complete = (IPC_RXMSG_HEADER_COMPLETE_MASK & header_byte1) >> 7;
  /* get msg size */
  header->size = (((uint16_t)IPC_RXMSG_HEADER_SIZE_MASK & (uint16_t)header_byte1) << 8);
  header->size = header->size + header_byte2;

  return;
}

static void RXFIFO_updateMsgHeader(IPC_Handle_t *hipc)
{
  /* update header with the size of last complete msg received */
  uint8_t header_byte1, header_byte2;
  uint16_t index;

  /* set header byte 1:  complete bit + size (upper part)*/
  header_byte1 = (uint8_t)(0x80U | ((hipc->RxQueue.current_msg_size >> 8) & 0x9FU));
  /* set header byte 2:  size (lower part)*/
  header_byte2 = (uint8_t)(hipc->RxQueue.current_msg_size & 0xFFU);

  /* write header bytes */
  index = hipc->RxQueue.current_msg_index;
  hipc->RxQueue.data[index] = header_byte1;
  index = (index + 1U) % IPC_RXBUF_MAXSIZE;
  hipc->RxQueue.data[index] = header_byte2;

#if (DBG_IPC_RX_FIFO != 0U)
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].complete = 1;
  hipc->dbgRxQueue.queue_pos = (hipc->dbgRxQueue.queue_pos + 1) % DBG_QUEUE_SIZE;
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].start_pos = hipc->RxQueue.current_msg_index;
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].complete = 0;
#endif /* DBG_IPC_RX_FIFO */

  return;
}

static void RXFIFO_prepareNextMsgHeader(IPC_Handle_t *hipc)
{
  uint8_t i;

  for (i = 0U; i < IPC_RXMSG_HEADER_SIZE; i++)
  {
    hipc->RxQueue.data[hipc->RxQueue.index_write] = 0U;
    RXFIFO_incrementHead(hipc);
  }

  return;
}

static void rearm_RX_IT(IPC_Handle_t *hipc)
{
  HAL_StatusTypeDef err ;

  /* Comment: specific to UART, should be in ipc_uart.c */
  /* rearm uart TX interrupt */
  if (hipc->Interface.interface_type == IPC_INTERFACE_UART)
  {
    err = HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)g_IPC_Devices_List[hipc->Device_ID].RxChar, 1U);
    if (err != HAL_OK)
    {
      hipc->UartBusyFlag = 1U;
    }
  }
}

#if (DBG_IPC_RX_FIFO != 0U)
void dump_RX_dbg_infos(IPC_Handle_t *hipc, uint8_t databuf, uint8_t queue)
{
  uint32_t i;

  if (databuf == 1)
  {
    PrintBuf((const char *)&hipc->RxQueue.data[0], IPC_RXBUF_MAXSIZE)
    /* PrintBufHexa((const char *)&hipc->RxQueue.data[0], IPC_RXBUF_MAXSIZE) */
  }

  PrintINFO("\r\n")

  if (queue == 1)
  {
    for (i = 0; i <= hipc->dbgRxQueue.queue_pos; i++)
    {
      PrintINFO(" [index %d]:  start_pos=%d size=%d complete=%d",
                i,
                hipc->dbgRxQueue.msg_info_queue[i].start_pos,
                hipc->dbgRxQueue.msg_info_queue[i].size,
                hipc->dbgRxQueue.msg_info_queue[i].complete)
    }
  }
}
#endif /* DBG_IPC_RX_FIFO */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

