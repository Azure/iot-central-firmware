/**
  ******************************************************************************
  * @file    at_datapack.h
  * @author  MCD Application Team
  * @brief   Header for at_datapack.c module
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
#ifndef AT_DATAPACK_H
#define AT_DATAPACK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"

/* Exported constants --------------------------------------------------------*/
#define DATAPACK_MAX_BUF_SIZE (ATCMD_MAX_BUF_SIZE)

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  DATAPACK_OK = 0,
  DATAPACK_ERROR,
} DataPack_Status_t;

typedef struct
{
  void *structptr;
} datapack_structptr_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
DataPack_Status_t DATAPACK_writePtr(uint8_t *p_buf, uint16_t msgtype, void *p_data);
DataPack_Status_t DATAPACK_writeStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data);
DataPack_Status_t DATAPACK_readPtr(uint8_t *p_buf, uint16_t msgtype, void **p_data);
DataPack_Status_t DATAPACK_readStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data);
uint16_t               DATAPACK_readMsgType(uint8_t *p_buf);
uint16_t               DATAPACK_readSize(uint8_t *p_buf);

#ifdef __cplusplus
}
#endif

#endif /* AT_DATAPACK_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
