/**
  ******************************************************************************
  * @file    com_sockets_err_compat.h
  * @author  MCD Application Team
  * @brief   Header for com_sockets_err_compat.c module
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
#ifndef COM_SOCKETS_ERR_COMPAT_H
#define COM_SOCKETS_ERR_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (COM_SOCKETS_ERRNO_COMPAT == 1) || (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "lwip/errno.h"
#endif

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Definitions for error constants */
typedef enum
{
  COM_SOCKETS_ERR_OK          =   0, /* Everything OK              */
  COM_SOCKETS_ERR_GENERAL     =  -1, /* General / Low level error  */
  COM_SOCKETS_ERR_DESCRIPTOR  =  -2, /* Invalid socket descriptor  */
  COM_SOCKETS_ERR_PARAMETER   =  -3, /* Invalid parameter          */
  COM_SOCKETS_ERR_WOULDBLOCK  =  -4, /* Operation would block      */
  COM_SOCKETS_ERR_NOMEMORY    =  -5, /* Out of memory error        */
  COM_SOCKETS_ERR_CLOSING     =  -6, /* Connection is closing      */
  COM_SOCKETS_ERR_LOCKED      =  -7, /* Address in use             */
  COM_SOCKETS_ERR_TIMEOUT     =  -8, /* Timeout                    */
  COM_SOCKETS_ERR_INPROGRESS  =  -9, /* Operation in progress      */
  COM_SOCKETS_ERR_NONAME      = -10, /* Host Name not existing     */

  /* Added value */
  COM_SOCKETS_ERR_NONETWORK   = -11, /* No network to proceed      */
  COM_SOCKETS_ERR_UNSUPPORTED = -12, /* Unsupported functionnality */
  COM_SOCKETS_ERR_STATE       = -13,  /* Connect rqt but Socket is already connected
                                        Send/Recv rqt but Socket is not connected
                                        ... */
} com_sockets_err_t;

/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/* Used only in com_sockets_ip_modem in getsockopt(SO_ERROR)
   to translate an error com_sockets enum to errno enum */
int32_t com_sockets_err_to_errno(com_sockets_err_t err);


#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_ERR_COMPAT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
