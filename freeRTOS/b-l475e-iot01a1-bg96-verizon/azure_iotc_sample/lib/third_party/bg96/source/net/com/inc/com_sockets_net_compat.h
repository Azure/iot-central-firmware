/**
  ******************************************************************************
  * @file    com_sockets_net_compat.h
  * @author  MCD Application Team
  * @brief   Com sockets net definition for compatibility LwIP / Modem
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
#ifndef COM_SOCKETS_NET_COMPAT_H
#define COM_SOCKETS_NET_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/* Exported constants --------------------------------------------------------*/
#define COM_INADDR_ANY          ((uint32_t)0x00000000UL) /* All IP adresses accepted */

/* Socket Family */
#define COM_AF_UNSPEC       0 /* Unspecified */
#define COM_AF_INET         1 /* Internet Address Family */
#define COM_AF_INET6        3 /* Internet Address Family version 6 */

/* Socket Type TCP/UDP/RAW */
#define COM_SOCK_STREAM     1 /* Supported */
#define COM_SOCK_DGRAM      2 /* Not supported */
#define COM_SOCK_RAW        3 /* Not supported */

/* Socket Protocol */
#define COM_IPPROTO_IP      0  /* IPv4 Level   */
#define COM_IPPROTO_ICMP    1
#define COM_IPPROTO_UDP     2  /* UDP Protocol */
#define COM_IPPROTO_IPV6    3  /* IPv6 Level   */
#define COM_IPPROTO_TCP     6  /* TCP Protocol */

/*
 * Socket Options
 */
/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define COM_SOL_SOCKET     0xfff  /* options for socket level */

#define COM_SO_SNDTIMEO    0x1005 /* send timeout */
#define COM_SO_RCVTIMEO    0x1006 /* receive timeout */
#define COM_SO_ERROR       0x1007 /* get error status and clear */

/* Flags used with recv. */
#define COM_MSG_WAIT       0x00    /* Blocking    */
#define COM_MSG_DONTWAIT   0x01    /* Nonblocking */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#else /* USE_SOCKETS_TYPE != USE_SOCKETS_MODEM */

/* Includes ------------------------------------------------------------------*/
/* only when USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
#include "lwip/sockets.h"

/* Exported constants --------------------------------------------------------*/
#define COM_INADDR_ANY      INADDR_ANY

/* Socket Family */
#define COM_AF_UNSPEC       AF_UNSPEC /* Unspecified */
#define COM_AF_INET         AF_INET   /* Internet Address Family */
#define COM_AF_INET6        AF_INET6  /* Internet Address Family version 6 */

/* Socket Type TCP/UDP/RAW */
#define COM_SOCK_STREAM     SOCK_STREAM
#define COM_SOCK_DGRAM      SOCK_DGRAM
#define COM_SOCK_RAW        SOCK_RAW

/* Socket Protocol */
#define COM_IPPROTO_IP      IPPROTO_IP
#define COM_IPPROTO_ICMP    IPPROTO_ICMP
#define COM_IPPROTO_UDP     IPPROTO_UDP
#define COM_IPPROTO_IPV6    IPPROTO_IPV6
#define COM_IPPROTO_TCP     IPPROTO_TCP

/*
 * Socket Options
 */
/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define COM_SOL_SOCKET     SOL_SOCKET  /* options for socket level */

#define COM_SO_SNDTIMEO    SO_SNDTIMEO /* send timeout */
#define COM_SO_RCVTIMEO    SO_RCVTIMEO /* receive timeout */
#define COM_SO_ERROR       SO_ERROR    /* get error status and clear */

/* Flags used with recv. */
#define COM_MSG_WAIT       MSG_WAIT      /* Blocking    */
#define COM_MSG_DONTWAIT   MSG_DONTWAIT  /* Nonblocking */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */


#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_NET_COMPAT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
