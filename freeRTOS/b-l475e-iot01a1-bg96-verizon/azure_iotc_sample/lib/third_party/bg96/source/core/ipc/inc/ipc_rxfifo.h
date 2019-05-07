/**
  ******************************************************************************
  * @file    ipc_rxfifo.h
  * @author  MCD Application Team
  * @brief   Header for ipc_rxfifo.c module
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
#ifndef IPC_RXFIFO_H
#define IPC_RXFIFO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void RXFIFO_init(IPC_Handle_t *hipc);
void RXFIFO_writeCharacter(IPC_Handle_t *hipc, uint8_t rxChar);
int16_t RXFIFO_read(IPC_Handle_t *hipc, IPC_RxMessage_t *o_Msg);
#if (IPC_USE_STREAM_MODE == 1U)
void RXFIFO_stream_init(IPC_Handle_t *hipc);
void RXFIFO_writeStream(IPC_Handle_t *hipc, uint8_t rxChar);
#endif /* IPC_USE_STREAM_MODE */

/* Debug functions */
void RXFIFO_print_data(IPC_Handle_t *hipc, uint16_t index, uint16_t size, uint8_t readable);
void RXFIFO_readMsgHeader_at_pos(IPC_Handle_t *hipc, IPC_RxHeader_t *header, uint16_t pos);
uint16_t RXFIFO_getFreeBytes(IPC_Handle_t *hipc);
#if (DBG_IPC_RX_FIFO != 0U)
void dump_RX_dbg_infos(IPC_Handle_t *hipc, uint8_t databuf, uint8_t queue);
#endif /* DBG_IPC_RX_FIFO */

#ifdef __cplusplus
}
#endif

#endif /* IPC_RXFIFO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

