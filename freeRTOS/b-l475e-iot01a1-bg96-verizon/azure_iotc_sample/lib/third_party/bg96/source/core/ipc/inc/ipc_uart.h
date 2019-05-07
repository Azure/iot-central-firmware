/**
  ******************************************************************************
  * @file    ipc_uart.h
  * @author  MCD Application Team
  * @brief   Header for ipc_uart.c module
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
#ifndef IPC_UART_H
#define IPC_UART_H

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
IPC_Status_t IPC_init_uart(IPC_Device_t  device, UART_HandleTypeDef  *huart);
IPC_Status_t IPC_deinit_uart(IPC_Device_t  device);
IPC_Status_t IPC_open_uart(IPC_Handle_t *hipc,
                           IPC_Device_t  device,
                           IPC_Mode_t    mode,
                           IPC_RxCallbackTypeDef pRxClientCallback,
                           IPC_TxCallbackTypeDef pTxClientCallback,
                           IPC_CheckEndOfMsgCallbackTypeDef pCheckEndOfMsg);
IPC_Status_t IPC_close_uart(IPC_Handle_t *hipc);
IPC_Status_t IPC_select_uart(IPC_Handle_t *hipc);
IPC_Status_t IPC_reset_uart(IPC_Handle_t *hipc);
IPC_Handle_t *IPC_get_other_channel_uart(IPC_Handle_t *hipc);
IPC_Status_t IPC_send_uart(IPC_Handle_t *hipc, uint8_t *p_TxBuffer, uint16_t bufsize);
IPC_Status_t IPC_receive_uart(IPC_Handle_t *hipc, IPC_RxMessage_t *p_msg);
IPC_Status_t IPC_streamReceive_uart(IPC_Handle_t *hipc,  uint8_t *p_buffer, int16_t *p_len);
void IPC_dump_RX_queue_uart(IPC_Handle_t *hipc, uint8_t readable);

void IPC_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_ErrorCallback(UART_HandleTypeDef *UartHandle);

#ifdef __cplusplus
}
#endif

#endif /* IPC_UART_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

