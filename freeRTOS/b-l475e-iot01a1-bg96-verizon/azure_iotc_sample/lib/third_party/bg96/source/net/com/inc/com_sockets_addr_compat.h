/**
  ******************************************************************************
  * @file    com_sockets_addr_compat.h
  * @author  MCD Application Team
  * @brief   Com sockets address definition for compatibility LwIP / Modem
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
#ifndef COM_SOCKETS_ADDR_COMPAT_H
#define COM_SOCKETS_ADDR_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/* Exported constants --------------------------------------------------------*/
#define COM_SOCKETS_LITTLE_ENDIAN (0)
#define COM_SOCKETS_BIG_ENDIAN    (1)

#ifndef COM_SOCKETS_BYTE_ORDER
#define COM_SOCKETS_BYTE_ORDER COM_SOCKETS_LITTLE_ENDIAN
#endif

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  COM_SOCKETS_FALSE = 0,
  COM_SOCKETS_TRUE  = 1
} com_bool_t;

typedef uint8_t com_char_t;

typedef struct
{
  uint32_t addr;
} com_ip4_addr_t;

typedef struct
{
  uint32_t s_addr;
} com_in_addr_t;

/* Only IPv4 is managed */
typedef com_ip4_addr_t com_ip_addr_t;

typedef struct
{
  uint8_t    sa_len;
  uint8_t    sa_family;
  com_char_t sa_data[14];
} com_sockaddr_t;

/* IPv4 Socket Address structure */
typedef struct
{
  uint8_t       sin_len;
  uint8_t       sin_family;
  uint16_t      sin_port;
  com_in_addr_t sin_addr;
  com_char_t    sin_zero[8];
} com_sockaddr_in_t;

typedef struct
{
  uint16_t time;
  uint16_t bytes;
  int32_t  status;
} com_ping_rsp_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#if (COM_SOCKETS_BYTE_ORDER == COM_SOCKETS_BIG_ENDIAN)

#define com_htonl(x)   (x)
#define com_ntohl(x)   (x)
#define com_htons(x)   (x)
#define com_ntohs(x)   (x)

#else /* COM_SOCKETS_BYTE_ORDER != COM_SOCKETS_BIG_ENDIAN */

#define com_htonl(x)   ((uint32_t)(((x) & 0xff000000U) >> 24) | \
                        (uint32_t)(((x) & 0x00ff0000U) >> 8) | \
                        (uint32_t)(((x) & 0x0000ff00U) << 8) | \
                        (uint32_t)(((x) & 0x000000ffU) << 24))

#define com_ntohl(x)    com_htonl(x)

#define com_htons(x)    ((uint16_t)(((x) & 0xff00U) >> 8) | \
                         (uint16_t)(((x) & 0x00ffU) << 8))

#define com_ntohs(x)    com_htons(x)


#endif /* COM_SOCKETS_BYTE_ORDER */

/* Format an uin32_t address */
#define COM_IP4_ADDR(ipaddr, a,b,c,d) \
        (ipaddr)->addr = com_htonl(((uint32_t)((uint32_t)((a) & 0xffU) << 24) | \
                                    (uint32_t)((uint32_t)((b) & 0xffU) << 16) | \
                                    (uint32_t)((uint32_t)((c) & 0xffU) << 8)  | \
                                    (uint32_t)((d) & 0xffU)))

/* Get one byte from the 4-byte address */
#define COM_IP4_ADDR1(ipaddr) (((const uint8_t*)(&(ipaddr)->addr))[0])
#define COM_IP4_ADDR2(ipaddr) (((const uint8_t*)(&(ipaddr)->addr))[1])
#define COM_IP4_ADDR3(ipaddr) (((const uint8_t*)(&(ipaddr)->addr))[2])
#define COM_IP4_ADDR4(ipaddr) (((const uint8_t*)(&(ipaddr)->addr))[3])

/* Exported functions ------------------------------------------------------- */

#else /* USE_SOCKETS_TYPE != USE_SOCKETS_MODEM */

/* Includes ------------------------------------------------------------------*/
/* only when USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
#include "lwip/sockets.h"
#include "lwip/ip4.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  COM_SOCKETS_FALSE = 0,
  COM_SOCKETS_TRUE  = 1
} com_bool_t;

typedef uint8_t com_char_t;

/* Re-Route com define to LwIP define */
typedef ip4_addr_t com_ip4_addr_t;
typedef ip_addr_t  com_ip_addr_t;
typedef in_addr_t  com_in_addr_t;
typedef struct sockaddr com_sockaddr_t;
typedef struct sockaddr_in com_sockaddr_in_t;

typedef struct
{
  uint16_t time;
  uint16_t bytes;
  int32_t  status;
} com_ping_rsp_t;

/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define com_htonl(A)   htonl((A))

#define com_ntohl(A)   ntonl((A))

#define com_htons(A)   htons((A))
#define com_ntohs(A)   htons((A))

#define COM_IP4_ADDR(ipaddr, a,b,c,d) IP4_ADDR((ipaddr), (a),(b),(c),(d))

/* Get one byte from the 4-byte address */
#define COM_IP4_ADDR1(ipaddr) ip4_addr1(ipaddr)
#define COM_IP4_ADDR2(ipaddr) ip4_addr2(ipaddr)
#define COM_IP4_ADDR3(ipaddr) ip4_addr3(ipaddr)
#define COM_IP4_ADDR4(ipaddr) ip4_addr4(ipaddr)


/* Exported functions ------------------------------------------------------- */

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */


#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_ADDR_COMPAT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
