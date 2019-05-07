/**
  ******************************************************************************
  * @file    com_sockets_ip_modem.h
  * @author  MCD Application Team
  * @brief   Header for com_sockets_ip_modem.c module
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
#ifndef COM_SOCKETS_IP_MODEM_H
#define COM_SOCKETS_IP_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "com_sockets_addr_compat.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/*** Global Restriction: Only TCP IPv4 client mode is supported ***************/

/*** Socket management ********************************************************/

/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  *         only TCP IPv4 client mode supported
  * @param  family   - address family
  * @note   only AF_INET supported
  * @param  type     - connection type
  * @note   only SOCK_STREAM supported
  * @param  protocol - protocol type
  * @note   only IPPROTO_TCP supported
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket_ip_modem(int32_t family, int32_t type, int32_t protocol);

/**
  * @brief  Socket option set
  * @note   Set option for the socket
  * @note   only send or receive timeout supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which the option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is to be set
  * @note   COM_SO_SNDTIMEO : OK but value not used because there is already
  *                           a tempo at low level - risk of conflict
  *         COM_SO_RCVTIMEO : OK
  *         any other value is rejected
  * @param  optval    - pointer to the buffer containing the option value
  * @note   COM_SO_SNDTIMEO and COM_SO_RCVTIMEO : unit is ms
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname,
                                const void *optval, int32_t optlen);

/**
  * @brief  Socket option get
  * @note   Get option for a socket
  * @note   only send timeout, receive timeout, last error supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is requested
  * @note   COM_SO_SNDTIMEO, COM_SO_RCVTIMEO, COM_SO_ERROR supported
  *         any other value is rejected
  * @param  optval    - pointer to the buffer that will contain the option value
  * @note   COM_SO_SNDTIMEO, COM_SO_RCVTIMEO: in ms for timeout (uint32_t)
  *         COM_SO_ERROR : result of last operation (int32_t)
  * @param  optlen    - size of the buffer that will contain the option value
  * @note   must be sizeof(x32_t)
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname,
                                void *optval, int32_t *optlen);

/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - local IP address and port
  * @note   only port value field is used as local port,
  *         but whole addr parameter must be "valid"
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind_ip_modem(int32_t sock,
                          const com_sockaddr_t *addr, int32_t addrlen);

/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         For an opened socket as long as socket close is in error value
  *         socket must be considered as not closed and handle as not released
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_ip_modem(int32_t sock);


/*** Client functionalities ***************************************************/

/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - remote IP address and port
  * @note   only an IPv4 address is supported
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect_ip_modem(int32_t sock,
                             const com_sockaddr_t *addr, int32_t addrlen);

/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @note   see below
  * @param  len       - size of the data to send (in bytes)
  * @note   see below
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         if len > interface between COM and low level
  *         a maximum of MODEM_MAX_TX_DATA_SIZE can be send
  *         - if flags = COM_MSG_WAIT, application accept to wait result
  *         of possible multiple send between COM and low level
  *         a buffer whose len > interface can be send
  *         COM will fragment the buffer according to the interface
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send_ip_modem(int32_t sock,
                          const com_char_t *buf, int32_t len,
                          int32_t flags);

/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @note   see below
  * @param  len       - size of application data buffer (in bytes)
  * @note   even if len > interface between COM and low level
  *         a maximum of the interface capacity can be received
  *         at each function call
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         until data are available at low level
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         until data are available at low level with respect of potential
  *         timeout COM_SO_RCVTIMEO setting
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv_ip_modem(int32_t sock,
                          com_char_t *buf, int32_t len,
                          int32_t flags);


/*** Server functionalities - NOT yet supported *******************************/

/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_listen_ip_modem(int32_t sock,
                            int32_t backlog);

/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - IP address and port number of the accepted connection
  * @param  len       - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_accept_ip_modem(int32_t sock,
                            com_sockaddr_t *addr, int32_t *addrlen);

/**
  * @brief  Socket send to data
  * @note   Send data to a remote host
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - length of the data to send (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_sendto_ip_modem(int32_t sock,
                            const com_char_t *buf, int32_t len,
                            int32_t flags,
                            const com_sockaddr_t *to, int32_t tolen);

/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_recvfrom_ip_modem(int32_t sock,
                              com_char_t *buf, int32_t len,
                              int32_t flags,
                              com_sockaddr_t *from, int32_t *fromlen);


/*** Other functionalities ****************************************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  None
  * @retval bool      - true/false init ok/nok
  */
com_bool_t com_init_ip_modem(void);

/**
  * @brief  Component start
  * @note   must be called only one time but
  *         after com_init and dc_start
  *         and before using any other functions of com_*
  * @param  None
  * @retval None
  */
void com_start_ip_modem(void);

/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         DNS resolver is a fix value in the module
  *         only a primary DNS is used
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @note   only IPv4 address is managed
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_ip_modem(const com_char_t *name,
                                   com_sockaddr_t   *addr);

/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getpeername_ip_modem(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen);

/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getsockname_ip_modem(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen);

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  None
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_ip_modem(void);

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
int32_t com_ping_process_ip_modem(int32_t ping,
                                  const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp);

/**
  * @brief  Ping close
  * @note   Close a ping session and release ping handle
  * @param  ping      - ping handle obtained with com_socket
  * @note   ping handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closeping_ip_modem(int32_t ping);

#endif /* USE_COM_PING */

#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_IP_MODEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
