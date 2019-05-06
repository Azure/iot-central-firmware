/**
  ******************************************************************************
  * @file    com_sockets_lwip_mcu.c
  * @author  MCD Application Team
  * @brief   This file implements Socket LwIP on MCU side
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
#include "com_sockets_lwip_mcu.h"
#include "plf_config.h"
#include "com_sockets_net_compat.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "error_handler.h"
#include "cmsis_os.h"

#include "lwip/sockets.h"
#include "lwip/api.h"     /* for netconn_gethostbyname */
#include "lwip/tcpip.h"   /* for tcp_init */
/* ICMP include for Ping */
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip4.h"

/* Private defines -----------------------------------------------------------*/
#if (USE_TRACE_COM_SOCKETS == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) \
TracePrint(DBG_CHAN_COM, DBL_LVL_P0, "COM: " format "\n\r", ## args)
#define PrintDBG(format, args...)  \
TracePrint(DBG_CHAN_COM, DBL_LVL_P1, "COM: " format "\n\r", ## args)
#define PrintERR(format, args...)  \
TracePrint(DBG_CHAN_COM, DBL_LVL_ERR, "COM ERROR: " format "\n\r", ## args)
#define PrintSTAT(format, args...) \
TracePrint(DBG_CHAN_COM, DBL_LVL_P0, "" format "\n\r", ## args)

#else /* USE_PRINTF == 1 */
#define PrintINFO(format, args...)  \
printf("COM: " format "\n\r", ## args);
#define PrintDBG(format, args...)   \
printf("COM: " format "\n\r", ## args);
#define PrintERR(format, args...)   \
printf("COM ERROR: " format "\n\r", ## args);
#define PrintSTAT(format, args...)  \
printf("" format "\n\r", ## args);

#endif

#else /* USE_TRACE_COM_SOCKETS == 0 */
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintERR(format, args...)   do {} while(0);
#define PrintSTAT(format, args...)  do {} while(0);
#endif /* USE_TRACE_COM_SOCKETS */

#if (USE_COM_PING == 1)
#define COM_PING_DATA_SIZE          32U
#define COM_PING_ID             0x1234U
#define COM_PING_MUTEX_TIMEOUT   10000U /* in ms : 1 sec. */
#define COM_PING_RSP_LEN_MAX        80U /* in bytes */
#endif /* USE_COM_PING */

/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Mutex to protect Ping process - only one Ping at a time is authorized */
/* only one Ping to avoid to many buffer allocation to send/receive message */
#if (USE_COM_PING == 1)
static osMutexId ComPingMutexHandle;

static struct icmp_echo_hdr *ping_snd;
static com_char_t *ping_rcv;
static uint8_t ping_seqno;
#endif /* USE_COM_PING */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/


/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  family   - address family
  * @param  type     - connection type
  * @param  protocol - protocol type
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket_lwip_mcu(int32_t family, int32_t type, int32_t protocol)
{
  return lwip_socket(family, type, protocol);
}

/**
  * @brief  Socket option set
  * @note   Set option for the socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which the option is defined
  * @param  optname   - option name for which the value is to be set
  * @param  optval    - pointer to the buffer containing the option value
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt_lwip_mcu(int32_t sock, int32_t level, int32_t optname,
                                const void *optval, int32_t optlen)
{
  return lwip_setsockopt(sock, level, optname,
                         optval, optlen);
}

/**
  * @brief  Socket option get
  * @note   Get option for a socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which option is defined
  * @param  optname   - option name for which the value is requested
  * @param  optval    - pointer to the buffer that will contain the option value
  * @param  optlen    - size of the buffer that will contain the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt_lwip_mcu(int32_t sock, int32_t level, int32_t optname,
                                void *optval, int32_t *optlen)
{
  return lwip_getsockopt(sock, level, optname,
                         optval, optlen);
}

/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - local IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind_lwip_mcu(int32_t sock,
                          const com_sockaddr_t *addr, int32_t addrlen)
{
  return lwip_bind(sock,
                   (const struct sockaddr *)addr, addrlen);
}

/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_lwip_mcu(int32_t sock)
{
  return lwip_close(sock);
}

/*** Client functionalities ***************************************************/

/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - remote IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect_lwip_mcu(int32_t sock,
                             const com_sockaddr_t *addr, int32_t addrlen)
{
  return lwip_connect(sock,
                      (const struct sockaddr *)addr, addrlen);
}

/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - size of the data to send (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send_lwip_mcu(int32_t sock,
                          const com_char_t *buf, int32_t len,
                          int32_t flags)
{
  return lwip_send(sock,
                   buf, (size_t)len,
                   flags);
}

/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv_lwip_mcu(int32_t sock,
                          com_char_t *buf, int32_t len,
                          int32_t flags)
{
  return lwip_recv(sock,
                   buf, (size_t)len,
                   flags);
}


/*** Server functionalities ***************************************************/

/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - ok or error value
  */
int32_t com_listen_lwip_mcu(int32_t sock,
                            int32_t backlog)
{
  return lwip_listen(sock,
                     backlog);
}

/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - IP address and port number of the accepted connection
  * @param  len       - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_accept_lwip_mcu(int32_t sock,
                            com_sockaddr_t *addr, int32_t *addrlen)
{
  return lwip_accept(sock,
                     (struct sockaddr *)addr, addrlen);
}

/**
  * @brief  Socket send to data
  * @note   Send data to a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - length of the data to send (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_sendto_lwip_mcu(int32_t sock,
                            const com_char_t *buf, int32_t len,
                            int32_t flags,
                            const com_sockaddr_t *to, int32_t tolen)
{
  return lwip_sendto(sock,
                     buf, (size_t)len,
                     flags,
                     (const struct sockaddr *)to, tolen);
}

/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recvfrom_lwip_mcu(int32_t sock,
                              com_char_t *buf, int32_t len,
                              int32_t flags,
                              com_sockaddr_t *from, int32_t *fromlen)
{
  return lwip_recvfrom(sock,
                       buf, (size_t)len,
                       flags,
                       (struct sockaddr *)from, fromlen);
}

/*** Other functionalities ****************************************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  *         Restrictions, if any, are linked to LwIP module used
  * @param  None
  * @retval bool      - true/false init ok/nok
  */
com_bool_t com_init_lwip_mcu(void)
{
  com_bool_t result;

  result = COM_SOCKETS_TRUE;

  /* No way to know if tcp_init is ok or not */
  tcpip_init(NULL, NULL);

#if (STACK_ANALYSIS_TRACE == 1)
  /* check values in FreeRTOSConfig.h */
  stackAnalysis_addStackSizeByHandle(TCPIP_THREAD_NAME,
                                     TCPIP_THREAD_STACKSIZE);
#endif /* STACK_ANALYSIS_TRACE */

#if (USE_COM_PING == 1)
  ping_snd   = NULL;
  ping_rcv   = NULL;
  ping_seqno = 0U;

  ping_snd = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)(sizeof(struct icmp_echo_hdr)
                                                             + COM_PING_DATA_SIZE));
  if (ping_snd == NULL)
  {
    result = COM_SOCKETS_FALSE;
    ERROR_Handler(DBG_CHAN_COM, 1, ERROR_FATAL);
  }
  else
  {
    ping_rcv = (uint8_t *)mem_malloc((mem_size_t)COM_PING_RSP_LEN_MAX);

    if (ping_rcv == NULL)
    {
      result = COM_SOCKETS_FALSE;
      ERROR_Handler(DBG_CHAN_COM, 2, ERROR_FATAL);
    }
    else
    {
      osMutexDef(ComPingMutex);
      ComPingMutexHandle = osMutexCreate(osMutex(ComPingMutex));
      if (ComPingMutexHandle == NULL)
      {
        result = COM_SOCKETS_FALSE;
        ERROR_Handler(DBG_CHAN_COM, 3, ERROR_FATAL);
      }
    }
    if (result == COM_SOCKETS_FALSE)
    {
      if (ping_snd != NULL)
      {
        mem_free(ping_snd);
        ping_snd = NULL;
      }
      if (ping_rcv != NULL)
      {
        mem_free(ping_rcv);
        ping_rcv = NULL;
      }
      if (ComPingMutexHandle != NULL)
      {
        osMutexDelete(ComPingMutexHandle);
        ComPingMutexHandle = NULL;
      }
    }
  }
#endif /* USE_COM_PING */

  return result;
}

/**
  * @brief  Component start
  * @note   must be called only one time but
  *         after com_init and dc_start
  *         and before using any other functions of com_*
  * @param  None
  * @retval None
  */
void com_start_lwip_mcu(void)
{
}

/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         Restrictions, if any, are linked to LwIP module used
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_lwip_mcu(const com_char_t *name,
                                   com_sockaddr_t   *addr)
{
  int32_t result;
  com_ip_addr_t ip_addr;

  result = netconn_gethostbyname((const char *)name, &ip_addr);
  if (result == 0)
  {
    memset(addr, 0, sizeof(com_sockaddr_t));
    addr->sa_family = (uint8_t)AF_UNSPEC;
    addr->sa_len    = sizeof(com_sockaddr_in_t);
    ((com_sockaddr_in_t *)addr)->sin_port = 0U;
    ((com_sockaddr_in_t *)addr)->sin_addr.s_addr = ip_addr.addr;
  }

  return result;
}

/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getpeername_lwip_mcu(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return lwip_getpeername(sock,
                          (struct sockaddr *)name, namelen);
}

/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockname_lwip_mcu(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return lwip_getsockname(sock,
                          (struct sockaddr *)name, namelen);
}

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  None
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_lwip_mcu(void)
{
  int32_t result;

  /* Only one Ping session at a time */
  if (osMutexWait(ComPingMutexHandle, COM_PING_MUTEX_TIMEOUT)
      == osOK)
  {
    result = com_socket_lwip_mcu(COM_AF_INET, COM_SOCK_RAW, COM_IPPROTO_ICMP);
    ping_seqno = 0U;
  }
  else
  {
    result = ERR_WOULDBLOCK;
  }

  return result;
}

/**
  * @brief  Ping process request
  * @note   Create a ping session
  * @param  ping     - ping handle obtained with com_ping
  * @note   ping handle on which operation has to be done
  * @param  addr     - remote IP address and port
  * @param  addrlen  - addr length
  * @param  timeout  - timeout for ping response (in sec)
  * @param  rsp      - ping response
  * @retval int32_t  - ok or error value
  */
int32_t com_ping_process_lwip_mcu(int32_t ping,
                                  const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp)
{
  int32_t result;
  uint32_t ping_size;
  /* manage the send to the distant */
  uint32_t timeout_ms;
  uint32_t ping_time;
  /* Manage the response from the distant */
  int32_t fromlen;
  com_sockaddr_in_t from;
  struct ip_hdr *p_ip_hdr;
  struct icmp_echo_hdr *p_icmp_echo;

  result = ERR_ARG;
  timeout_ms = (uint32_t)timeout * 1000U;
  ping_size = sizeof(struct icmp_echo_hdr) + COM_PING_DATA_SIZE;

  if ((ping >= 0)
      && (rsp != NULL)
      && (addr != NULL))
  {
    rsp->time  = 0U;
    rsp->bytes = 0U;

    if ((ping_snd != NULL)
        && (ping_rcv != NULL))
    {
      result = com_setsockopt_lwip_mcu(ping,
                                       COM_SOL_SOCKET, COM_SO_RCVTIMEO,
                                       &timeout_ms, (int32_t)sizeof(timeout_ms));
      if (result == ERR_OK)
      {
        /* Format icmp message to send */
        ICMPH_TYPE_SET(ping_snd, ICMP_ECHO);
        ICMPH_CODE_SET(ping_snd, 0);
        ping_snd->chksum = 0U;
        ping_snd->id     = COM_PING_ID;
        ping_seqno++;
        ping_snd->seqno  = ping_seqno;

        /* Fill the additional data buffer with some data */
        for (uint8_t i = sizeof(struct icmp_echo_hdr);
             i < COM_PING_DATA_SIZE; i++)
        {
          ((char *)ping_snd)[i] = (char)i;
        }

        ping_snd->chksum = inet_chksum(ping_snd,
                                       sizeof(struct icmp_echo_hdr) + COM_PING_DATA_SIZE);

        result = com_sendto_lwip_mcu(ping,
                                     (const com_char_t *)ping_snd, (int32_t)ping_size,
                                     0,
                                     (const com_sockaddr_t *)addr, addrlen);
        ping_time = sys_now();
        if (result == (int32_t)ping_size)
        {
          PrintDBG("ping request send")
          /* Send is OK, wait reponse from remote */
          result = com_recvfrom_lwip_mcu(ping,
                                         (uint8_t *)ping_rcv,
                                         (int32_t)COM_PING_RSP_LEN_MAX,
                                         0,
                                         (com_sockaddr_t *)&from, &fromlen);

          rsp->time  = (uint16_t)(sys_now() - ping_time);
          rsp->bytes = COM_PING_DATA_SIZE;
          /* Data has been received ? */
          if (result < 0)
          {
            /* Timeout - No response */
            result = ERR_TIMEOUT;
          }
          else
          {
            if ((uint32_t)result >= (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
            {
              PrintDBG("ping response received")
              p_ip_hdr = (struct ip_hdr *)ping_rcv;
              p_icmp_echo = (struct icmp_echo_hdr *)(ping_rcv + (IPH_HL(p_ip_hdr) * 4));
              if ((p_icmp_echo->id == COM_PING_ID)
                  && (p_icmp_echo->seqno == ping_seqno))
              {
                if (ICMPH_TYPE(p_icmp_echo) == (uint8_t)ICMP_ER)
                {
                  PrintDBG("ICMP ECHO REPLY received")
                  result = ERR_OK;
                  rsp->status = ERR_OK;
                }
                else
                {
                  result = ERR_IF;
                  rsp->status = (int32_t)ICMPH_TYPE(p_icmp_echo);
                }
              }
              else
              {
                result = ERR_IF;
              }
            }
          }
        }
      }
    }
    else
    {
      result = ERR_MEM;
    }
  }
  return result;
}

/**
  * @brief  Ping close
  * @note   Close a ping session and release ping handle
  * @param  ping      - ping handle obtained with com_socket
  * @note   ping handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closeping_lwip_mcu(int32_t ping)
{
  int32_t result;

  result = com_closesocket_lwip_mcu(ping);
  if (result == ERR_OK)
  {
    osMutexRelease(ComPingMutexHandle);
    ping_seqno = 0U;
  }

  return result;
}

#endif /* USE_COM_PING */


#endif /* USE_SOCKETS_TYPE */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
