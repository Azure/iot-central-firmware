/**
  ******************************************************************************
  * @file    ipc_config.h
  * @author  MCD Application Team
  * @brief   This file defines IPC Configuration
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
#ifndef IPC_CONFIG_H
#define IPC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "plf_features.h"

#define IPC_BUFFER_EXT    ((uint16_t) 400U) /* size addded to RX buffer because of RX queue implementation (using
                                            * headers for messages)
                                            */
#define IPC_RXBUF_MAXSIZE ((uint16_t) 1600U + IPC_BUFFER_EXT) /* maximun size of character queue
                                                              * size has to match ATCMD_MAX_CMD_SIZE
                                                              */

/* IPC tuning parameters */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
/* SOCKET MODE (IP stack in the modem) */
#define IPC_USE_STREAM_MODE (0U)
#else
/*  STREAM MODE (IP stack in MCU) */
#define IPC_USE_STREAM_MODE (1U)
#define IPC_RXBUF_STREAM_MAXSIZE  ((uint16_t) IPC_RXBUF_MAXSIZE) /* maximum size of stream queue (if used) */
#endif

/* IPC_RXBUF_MAXSIZE and IPC_RXBUF_STREAM_MAXSIZE are defined above */
#define IPC_RXBUF_THRESHOLD  ((uint16_t) 20U)

/* IPC interface */
#define IPC_USE_UART (1U) /* UART activated by default */
#define IPC_USE_SPI  (0U) /* SPI NOT SUPPORTED YET */
#define IPC_USE_I2C  (0U) /* I2C NOT SUPPORTED YET */

/* Debug flags */
#define DBG_IPC_RX_FIFO  (0U)             /* additional debug infos */
#define DBG_QUEUE_SIZE ((uint16_t) 1000U) /* debug message history depth */

#ifdef __cplusplus
}
#endif

#endif /* IPC_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
