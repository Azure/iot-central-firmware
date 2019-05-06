/**
  ******************************************************************************
  * @file    com_sockets_ip_modem.c
  * @author  MCD Application Team
  * @brief   This file implements Socket IP on MODEM side
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
#include "com_sockets_ip_modem.h"
#include "plf_config.h"

#include <string.h>

#include "com_sockets_net_compat.h"
#include "com_sockets_err_compat.h"

#include "cellular_service_os.h"

#if (USE_DATACACHE == 1)
#include "dc_common.h"
#include "dc_time.h"
#include "cellular_init.h"
#endif

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

/* Need a function in low level to obtain these values according to the modem */
/* Maximum data that can be passed between COM and low level */
#define MODEM_MAX_TX_DATA_SIZE ((uint32_t) 1460U)
#define MODEM_MAX_RX_DATA_SIZE ((uint32_t) 1500U)

#define COM_SOCKET_INVALID_ID (-1)

/* Socket statistic print */
/* see COM_SOCKET_STAT_POLLING_PERIOD for the print period */
#define COM_SOCKET_STATISTIC (1U) /* 0: not activated,
                                     1: activated */
/* Socket local id number : 1 for ping */
#define COM_SOCKET_LOCAL_ID_NB 1U

/* Private typedef -----------------------------------------------------------*/

/* Type of socket statistic printed */
typedef enum
{
  COM_SOCKET_STAT_INIT = 0,
#if (USE_DATACACHE == 1)
  COM_SOCKET_STAT_NWK_UP,
  COM_SOCKET_STAT_NWK_DWN,
#endif
  COM_SOCKET_STAT_CRE_OK,
  COM_SOCKET_STAT_CRE_NOK,
  COM_SOCKET_STAT_CNT_OK,
  COM_SOCKET_STAT_CNT_NOK,
  COM_SOCKET_STAT_SND_OK,
  COM_SOCKET_STAT_SND_NOK,
  COM_SOCKET_STAT_RCV_OK,
  COM_SOCKET_STAT_RCV_NOK,
  COM_SOCKET_STAT_CLS_OK,
  COM_SOCKET_STAT_CLS_NOK,
  COM_SOCKET_STAT_PRINT
} com_socket_stat_t;

#if (COM_SOCKET_STATISTIC == 1U)
/* Private defines -----------------------------------------------------------*/
#define COM_SOCKET_STAT_POLLING_PERIOD 10000U /* Statistic print period in ms */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
#if (USE_DATACACHE == 1)
  uint16_t nwk_up;
  uint16_t nwk_dwn;
#endif
  uint16_t sock_cre_ok;
  uint16_t sock_cre_nok;
  uint16_t sock_cnt_ok;
  uint16_t sock_cnt_nok;
  uint16_t sock_snd_ok;
  uint16_t sock_snd_nok;
  uint16_t sock_rcv_ok;
  uint16_t sock_rcv_nok;
  uint16_t sock_cls_ok;
  uint16_t sock_cls_nok;
} com_socket_statistic_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Statistic print timer */
static osTimerId com_socket_stat_timer_handle;

/* Statistic socket state readable print */
static const uint8_t *socket_state_string[] =
{
  "Invalid",
  "Creating",
  "Created",
  "Connected",
  "Sending",
  "Receiving",
  "Closing"
};


/* Statistic socket variable */
static com_socket_statistic_t com_socket_statistic;

#endif /* COM_SOCKET_STATISTIC */

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  SOCKET_MSG  = 0,
  PING_MSG
} socket_msg_type_t;
typedef enum
{
  DATA_RCV    = 0,
  CLOSING_RCV
} socket_msg_id_t;

typedef struct
{
  uint16_t type;
  uint16_t id;
} socket_msg_t;

typedef enum
{
  SOCKET_INVALID = 0,
  SOCKET_CREATING,
  SOCKET_CREATED,
  SOCKET_CONNECTED,
  SOCKET_SENDING,
  SOCKET_WAITING_RSP,
  SOCKET_CLOSING
} socket_state_t;

typedef struct _socket_desc_t
{
  socket_state_t        state;       /* socket state */
  com_bool_t            local;       /*   internal id - e.g for ping
                                       or external id - e.g modem    */
  com_bool_t            closing;     /* close recv from remote  */
  int8_t                error;       /* last command status     */
  int32_t               id;          /* identifier */
  uint16_t              port;        /* local port */
  uint32_t              snd_timeout; /* timeout for send cmd    */
  uint32_t              rcv_timeout; /* timeout for receive cmd */
  osMessageQId          queue;       /* message queue for URC   */
  com_ping_rsp_t        *rsp;
  struct _socket_desc_t *next;       /* chained list            */
} socket_desc_t;

typedef struct
{
  CS_IPaddrType_t ip_type;
  /* In cellular_service socket_configure_remote()
     memcpy from char *addr to addr[] without knowing the length
     and by using strlen(char *addr) so ip_value must contain /0 */
  /* IPv4 : xxx.xxx.xxx.xxx=15+/0*/
  /* IPv6 : xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx = 39+/0*/
  uint8_t         ip_value[40];
  uint16_t        port;
} socket_addr_t;

/* Private macros ------------------------------------------------------------*/

#define COM_MIN(a,b) ((a)<(b) ? (a) : (b))

#define SOCKET_SET_ERROR(socket, val) do {\
  if ((socket) != NULL) {\
    (socket)->error = (int8_t)(val); }\
  } while(0)

#define SOCKET_GET_ERROR(socket, val) do {\
  if ((socket) != NULL) {\
    (*(int32_t *)(val)) = (((socket)->error) == COM_SOCKETS_ERR_OK ? \
             (int32_t)COM_SOCKETS_ERR_OK : \
             com_sockets_err_to_errno((com_sockets_err_t)((socket)->error))); }\
  else {\
    (*(int32_t *)(val)) = \
             com_sockets_err_to_errno(COM_SOCKETS_ERR_DESCRIPTOR); }\
  } while(0)


/* Private variables ---------------------------------------------------------*/

/* Mutex to protect access to socket descriptor list and local_id */
static osMutexId ComSocketsMutexHandle;
/* List socket descriptor */
static socket_desc_t *socket_desc_list;
/* Provide a socket local id */
static com_bool_t socket_local_id[COM_SOCKET_LOCAL_ID_NB]; /* false : unused
                                                              true  : in use  */
#if (USE_COM_PING == 1)
/* Ping socket id */
static int32_t ping_socket_id;
#endif /* USE_COM_PING */

#if (USE_DATACACHE == 1)
static com_bool_t network_is_up;
#endif

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Callback prototype */
#if (COM_SOCKET_STATISTIC == 1U)
static void com_socket_stat_timer_cb(void const *argument);
#endif

static void com_ip_modem_data_ready_cb(socket_handle_t sock);
static void com_ip_modem_closing_cb(socket_handle_t sock);

#if (USE_DATACACHE == 1)
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id,
                                    void *private_gui_data);
#endif

#if (USE_COM_PING == 1)
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp);
#endif

static void com_socket_stat_update(com_socket_stat_t stat);
static void com_ip_modem_init_socket_desc(socket_desc_t *socket_desc);
static socket_desc_t *com_ip_modem_create_socket_desc(void);
static socket_desc_t *com_ip_modem_provide_socket_desc(com_bool_t local);
static void com_ip_modem_delete_socket_desc(int32_t    sock,
                                            com_bool_t local);
static socket_desc_t *com_ip_modem_find_socket(int32_t    sock,
                                               com_bool_t local);
static com_bool_t com_translate_ip_address(const com_sockaddr_t *addr,
                                           int32_t       addrlen,
                                           socket_addr_t *socket_addr);
static com_bool_t com_convert_IPString_to_sockaddr(com_char_t     *ipaddr_str,
                                                   com_sockaddr_t *sockaddr);
static com_bool_t com_ip_modem_is_network_up(void);

/* Private function Definition -----------------------------------------------*/

#if (COM_SOCKET_STATISTIC == 1U)
/**
  * @brief  Managed com socket statistic update and print
  * @note   -
  * @param  stat - to know what the function has to do
  * @note   statistic init, update or print
  * @retval None
  */
static void com_socket_stat_update(com_socket_stat_t stat)
{
  switch (stat)
  {
#if (USE_DATACACHE == 1)
    case COM_SOCKET_STAT_NWK_UP:
    {
      com_socket_statistic.nwk_up++;
      break;
    }
    case COM_SOCKET_STAT_NWK_DWN:
    {
      com_socket_statistic.nwk_dwn++;
      break;
    }
#endif /* USE_DATACACHE */
    case COM_SOCKET_STAT_CRE_OK:
    {
      com_socket_statistic.sock_cre_ok++;
      break;
    }
    case COM_SOCKET_STAT_CRE_NOK:
    {
      com_socket_statistic.sock_cre_nok++;
      break;
    }
    case COM_SOCKET_STAT_CNT_OK:
    {
      com_socket_statistic.sock_cnt_ok++;
      break;
    }
    case COM_SOCKET_STAT_CNT_NOK:
    {
      com_socket_statistic.sock_cnt_nok++;
      break;
    }
    case COM_SOCKET_STAT_SND_OK:
    {
      com_socket_statistic.sock_snd_ok++;
      break;
    }
    case COM_SOCKET_STAT_SND_NOK:
    {
      com_socket_statistic.sock_snd_nok++;
      break;
    }
    case COM_SOCKET_STAT_RCV_OK:
    {
      com_socket_statistic.sock_rcv_ok++;
      break;
    }
    case COM_SOCKET_STAT_RCV_NOK:
    {
      com_socket_statistic.sock_rcv_nok++;
      break;
    }
    case COM_SOCKET_STAT_CLS_OK:
    {
      com_socket_statistic.sock_cls_ok++;
      break;
    }
    case COM_SOCKET_STAT_CLS_NOK:
    {
      com_socket_statistic.sock_cls_nok++;
      break;
    }
    case COM_SOCKET_STAT_INIT:
    {
      memset(&com_socket_statistic, 0, sizeof(com_socket_statistic_t));
      if (COM_SOCKET_STAT_POLLING_PERIOD != 0U)
      {
        osTimerDef(com_socket_stat_timer,
                   com_socket_stat_timer_cb);
        com_socket_stat_timer_handle = osTimerCreate(osTimer(com_socket_stat_timer),
                                                     osTimerPeriodic, NULL);
        osTimerStart(com_socket_stat_timer_handle,
                     COM_SOCKET_STAT_POLLING_PERIOD);
      }
      break;
    }
    case COM_SOCKET_STAT_PRINT:
    {
      socket_desc_t *socket_desc;
      socket_desc = socket_desc_list;

      /* Check that at least one socket has run */
      if (com_socket_statistic.sock_cre_ok != 0U)
      {
        PrintSTAT("*** Begin Socket Statistic ***")
#if (USE_DATACACHE == 1)
        dc_time_date_rt_info_t dc_time_date_rt_info;

        (void)dc_srv_get_time_date(&dc_time_date_rt_info,
                                   DC_DATE_AND_TIME);

        PrintSTAT("%02d/%02d/%04d - %02d:%02d:%02d",
                  dc_time_date_rt_info.mday,
                  dc_time_date_rt_info.month,
                  dc_time_date_rt_info.year,
                  dc_time_date_rt_info.hour,
                  dc_time_date_rt_info.min,
                  dc_time_date_rt_info.sec)

        PrintSTAT("Network: up: %5d dwn:  %5d tot: %6d",
                  com_socket_statistic.nwk_up,
                  com_socket_statistic.nwk_dwn,
                  (com_socket_statistic.nwk_up + com_socket_statistic.nwk_dwn))
#endif /* USE_DATACACHE */
        PrintSTAT("Create : ok: %5d nok:  %5d tot: %6d",
                  com_socket_statistic.sock_cre_ok,
                  com_socket_statistic.sock_cre_nok,
                  (com_socket_statistic.sock_cre_ok + com_socket_statistic.sock_cre_nok))
        PrintSTAT("Connect: ok: %5d nok:  %5d tot: %6d",
                  com_socket_statistic.sock_cnt_ok,
                  com_socket_statistic.sock_cnt_nok,
                  (com_socket_statistic.sock_cnt_ok + com_socket_statistic.sock_cnt_nok))
        PrintSTAT("Send   : ok: %5d nok:  %5d tot: %6d",
                  com_socket_statistic.sock_snd_ok,
                  com_socket_statistic.sock_snd_nok,
                  (com_socket_statistic.sock_snd_ok + com_socket_statistic.sock_snd_nok))
        PrintSTAT("Recv   : ok: %5d nok:  %5d tot: %6d",
                  com_socket_statistic.sock_rcv_ok,
                  com_socket_statistic.sock_rcv_nok,
                  (com_socket_statistic.sock_rcv_ok + com_socket_statistic.sock_rcv_nok))
        PrintSTAT("Close  : ok: %5d nok:  %5d tot: %6d",
                  com_socket_statistic.sock_cls_ok,
                  com_socket_statistic.sock_cls_nok,
                  (com_socket_statistic.sock_cls_ok + com_socket_statistic.sock_cls_nok))
        while (socket_desc != NULL)
        {
          PrintSTAT("Sock Id: %2d - State: %s - Status: %d",
                    socket_desc->id,
                    socket_state_string[socket_desc->state],
                    socket_desc->error)
          socket_desc = socket_desc->next;
        }
        PrintSTAT("*** End   Socket Statistic ***")
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

/**
  * @brief  Called when statistic timer raised
  * @note   Request com socket statistic print
  * @param  argument - parameter passed at creation of osTimer
  * @note   Unused
  * @retval None
  */
static void com_socket_stat_timer_cb(void const *argument)
{
  /* UNUSED(argument); */
  com_socket_stat_update(COM_SOCKET_STAT_PRINT);
}

#else /* COM_SOCKET_STATISTIC != 1 */
/**
  * @brief  Managed com socket statistic update and print
  * @note   empty function when COM_SOCKET_STATISTIC not activated
  * @param  stat - to know what the function has to do
  * @note   Unused
  * @retval None
  */
static void com_socket_stat_update(com_socket_stat_t stat)
{
  /* UNUSED(stat); */
}
#endif /* COM_SOCKET_STATISTIC */

static void com_ip_modem_init_socket_desc(socket_desc_t *socket_desc)
{
  socket_desc->state       = SOCKET_INVALID;
  socket_desc->local       = COM_SOCKETS_FALSE;
  socket_desc->closing     = COM_SOCKETS_FALSE;
  socket_desc->id          = COM_SOCKET_INVALID_ID;
  socket_desc->port        = 0U;
  socket_desc->rcv_timeout = osWaitForever;
  socket_desc->snd_timeout = osWaitForever;
  socket_desc->error       = COM_SOCKETS_ERR_OK;
}

static socket_desc_t *com_ip_modem_create_socket_desc(void)
{
  socket_desc_t *socket_desc;

  socket_desc = (socket_desc_t *)pvPortMalloc(sizeof(socket_desc_t));
  if (socket_desc != NULL)
  {
    osMessageQDef(MODEM_QUEUE, 2, uint32_t);
    socket_desc->queue = osMessageCreate(osMessageQ(MODEM_QUEUE), NULL);
    if (socket_desc->queue == NULL)
    {
      vPortFree(socket_desc);
    }
    else
    {
      com_ip_modem_init_socket_desc(socket_desc);
    }
  }

  return socket_desc;
}

static socket_desc_t *com_ip_modem_provide_socket_desc(com_bool_t local)
{
  osMutexWait(ComSocketsMutexHandle, osWaitForever);

  com_bool_t found;
  uint8_t i;
  socket_desc_t *socket_desc;
  socket_desc_t *socket_desc_previous;

  i = 0U;
  found = COM_SOCKETS_FALSE;
  socket_desc = socket_desc_list;
  socket_desc_previous = socket_desc_list;

  /* If socket is local first check an id is still available in the table */
  if (local == COM_SOCKETS_TRUE)
  {
    while ((i < COM_SOCKET_LOCAL_ID_NB)
           && (found == COM_SOCKETS_FALSE))
    {
      if (socket_local_id[i] == COM_SOCKETS_FALSE)
      {
        /* an unused local id has been found */
        found = COM_SOCKETS_TRUE;
        /* com_socket_local_id book is done only if socket is really created */
      }
    }
  }
  else
  {
    /* if local == false no specific treatment to do */
    found = COM_SOCKETS_TRUE;
  }

  if (found == COM_SOCKETS_TRUE)
  {
    /* Need to create a new socket_desc ? */
    while ((socket_desc->state != SOCKET_INVALID)
           && (socket_desc->next  != NULL))
    {
      /* Check next descriptor */
      socket_desc_previous = socket_desc;
      socket_desc = socket_desc->next;
    }
    /* Find an empty socket ? */
    if (socket_desc->state != SOCKET_INVALID)
    {
      /* No empty socket - create new one */
      socket_desc = com_ip_modem_create_socket_desc();
      if (socket_desc != NULL)
      {
        socket_desc->state = SOCKET_CREATING;
        /* attach new socket at the end of descriptor list */
        socket_desc_previous->next = socket_desc;
        socket_desc->local = local;
        if (local == COM_SOCKETS_TRUE)
        {
          /* even if an application create two sockets one local - one distant
             no issue if id is same because it will be stored
             in two variable s at application level */
          /* Don't need an OFFSET to not overlap Modem id */
          socket_desc->id = (int32_t)i;
          /*  Socket is really created - book com_socket_local_id */
          socket_local_id[i] = COM_SOCKETS_TRUE;
        }
      }
    }
    else
    {
      /* Find an empty place */
      socket_desc->state = SOCKET_CREATING;
      socket_desc->local = local;
      if (local == COM_SOCKETS_TRUE)
      {
        /* even if an application create two sockets one local - one distant
           no issue if id is same because it will be stored
           in two variable s at application level */
        /* Don't need an OFFSET to not overlap Modem id */
        socket_desc->id = (int32_t)i;
        /*  Socket is really created - book com_socket_local_id */
        socket_local_id[i] = COM_SOCKETS_TRUE;
      }
    }
  }

  osMutexRelease(ComSocketsMutexHandle);

  /* If provide == false then socket_desc = NULL */
  return socket_desc;
}

static void com_ip_modem_delete_socket_desc(int32_t    sock,
                                            com_bool_t local)
{
  osMutexWait(ComSocketsMutexHandle, osWaitForever);

  com_bool_t found;
  socket_desc_t *socket_desc;

  found = COM_SOCKETS_FALSE;

  socket_desc = socket_desc_list;

  while ((socket_desc != NULL)
         && (found != COM_SOCKETS_TRUE))
  {
    if ((socket_desc->id == sock)
        && (socket_desc->local == local))
    {
      found = COM_SOCKETS_TRUE;
    }
    else
    {
      socket_desc = socket_desc->next;
    }
  }
  if (found == COM_SOCKETS_TRUE)
  {
    /* Always keep a created socket */
    com_ip_modem_init_socket_desc(socket_desc);
    if (local == COM_SOCKETS_TRUE)
    {
      /* Free com_socket_local_id */
      socket_local_id[sock] = COM_SOCKETS_FALSE;
    }
  }

  osMutexRelease(ComSocketsMutexHandle);
}

static socket_desc_t *com_ip_modem_find_socket(int32_t    sock,
                                               com_bool_t local)
{
  socket_desc_t *socket_desc;
  com_bool_t found;

  socket_desc = socket_desc_list;
  found = COM_SOCKETS_FALSE;

  while ((socket_desc != NULL)
         && (found != COM_SOCKETS_TRUE))
  {
    if ((socket_desc->id == sock)
        && (socket_desc->local == local))
    {
      found = COM_SOCKETS_TRUE;
    }
    else
    {
      socket_desc = socket_desc->next;
    }
  }

  /* If found == false then socket_desc = NULL */
  return socket_desc;
}

static com_bool_t com_translate_ip_address(const com_sockaddr_t *addr,
                                           int32_t       addrlen,
                                           socket_addr_t *socket_addr)
{
  com_bool_t result;
  const com_sockaddr_in_t *p_sockaddr_in;

  result = COM_SOCKETS_FALSE;
  p_sockaddr_in = (const com_sockaddr_in_t *)addr;

  if ((addr != NULL)
      && (socket_addr != NULL))
  {
    if (addr->sa_family == (uint8_t)COM_AF_INET)
    {
      socket_addr->ip_type = CS_IPAT_IPV4;
      if (p_sockaddr_in->sin_addr.s_addr == COM_INADDR_ANY)
      {
        memcpy(&socket_addr->ip_value[0], "0.0.0.0", strlen("0.0.0.0"));
      }
      else
      {
        com_ip_addr_t com_ip_addr;

        com_ip_addr.addr = ((const com_sockaddr_in_t *)addr)->sin_addr.s_addr;
        sprintf((char *)&socket_addr->ip_value[0], "%d.%d.%d.%d",
                COM_IP4_ADDR1(&com_ip_addr),
                COM_IP4_ADDR2(&com_ip_addr),
                COM_IP4_ADDR3(&com_ip_addr),
                COM_IP4_ADDR4(&com_ip_addr));
      }
      socket_addr->port = com_ntohs(((const com_sockaddr_in_t *)addr)->sin_port);
      result = COM_SOCKETS_TRUE;
    }
    /* else if (addr->sa_family == COM_AF_INET6) */
    /* or any other value */
    /* not supported */
  }

  return result;
}

static com_bool_t com_convert_IPString_to_sockaddr(com_char_t     *ipaddr_str,
                                                   com_sockaddr_t *sockaddr)
{
  com_bool_t result;
  int32_t  count;
  uint8_t  begin;
  uint32_t  ip_addr[4];
  com_ip4_addr_t ip4_addr;

  result = COM_SOCKETS_FALSE;
  begin = (ipaddr_str[0] == '"') ? 1U : 0U;

  count = sscanf((char *)(&ipaddr_str[begin]), "%03lu.%03lu.%03lu.%03lu",
                 &ip_addr[0], &ip_addr[1],
                 &ip_addr[2], &ip_addr[3]);

  if (count == 4)
  {
    if ((ip_addr[0] <= 255U)
        && (ip_addr[1] <= 255U)
        && (ip_addr[2] <= 255U)
        && (ip_addr[3] <= 255U))
    {
      memset(sockaddr, 0, sizeof(com_sockaddr_t));
      sockaddr->sa_family = (uint8_t)COM_AF_INET;
      sockaddr->sa_len    = sizeof(com_sockaddr_in_t);
      ((com_sockaddr_in_t *)sockaddr)->sin_port = 0U;
      COM_IP4_ADDR(&ip4_addr, ip_addr[0], ip_addr[1],
                   ip_addr[2], ip_addr[3]);
      ((com_sockaddr_in_t *)sockaddr)->sin_addr.s_addr = ip4_addr.addr;
      result = COM_SOCKETS_TRUE;
    }
  }

  return (result);
}

static com_bool_t com_ip_modem_is_network_up(void)
{
  com_bool_t result;

#if (USE_DATACACHE == 1)
  result = network_is_up;
#else
  /* Feature not supported without Datacache
     Do not block -=> consider network is up */
  result = COM_SOCKETS_TRUE;
#endif /* USE_DATACACHE */

  return result;
}

/**
  * @brief  Callback called when URC data ready raised
  * @note   Managed URC data ready
  * @param  sock - socket handle
  * @note   -
  * @retval None
  */
static void com_ip_modem_data_ready_cb(socket_handle_t sock)
{
  PrintDBG("callback socket data ready called")

  socket_msg_t   msg;
  socket_msg_t  *p_msg;
  socket_desc_t *socket_desc;

  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if ((socket_desc != NULL)
      && (socket_desc->closing == COM_SOCKETS_FALSE)
      && (socket_desc->state   == SOCKET_WAITING_RSP))
  {
    PrintINFO("callback socket data ready called: data ready")
    msg.type = SOCKET_MSG;
    msg.id   = DATA_RCV;
    p_msg    = &msg;
    osMessagePut(socket_desc->queue, *((uint32_t *)p_msg), 0U);
  }
  else
  {
    PrintINFO("!!! PURGE callback socket data ready !!!")
    if (socket_desc == NULL)
    {
      PrintERR("callback socket data ready called: unknown socket")
    }
    if (socket_desc->closing == COM_SOCKETS_TRUE)
    {
      PrintERR("callback socket data ready called: socket is closing")
    }
    if (socket_desc->state != SOCKET_WAITING_RSP)
    {
      PrintERR("callback socket data ready called: socket_state:%i NOK",
               socket_desc->state)
    }
  }
}

/**
  * @brief  Callback called when URC socket closing raised
  * @note   Managed URC socket closing
  * @param  sock - socket handle
  * @note   -
  * @retval None
  */
static void com_ip_modem_closing_cb(socket_handle_t sock)
{
  PrintDBG("callback socket closing called")

  socket_msg_t   msg;
  socket_msg_t  *p_msg;
  socket_desc_t *socket_desc;

  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);
  if (socket_desc != NULL)
  {
    PrintINFO("callback socket closing called: close rqt")
    if (socket_desc->closing == COM_SOCKETS_FALSE)
    {
      socket_desc->closing = COM_SOCKETS_TRUE;
      PrintINFO("callback socket closing: close rqt")
    }
    if (socket_desc->state == SOCKET_WAITING_RSP)
    {
      PrintERR("!!! callback socket closing called: data_expected !!!")
      msg.type = SOCKET_MSG;
      msg.id   = CLOSING_RCV;
      p_msg    = &msg;
      osMessagePut(socket_desc->queue, *((uint32_t *)p_msg), 0U);
    }
  }
  else
  {
    PrintERR("callback socket closing called: unknown socket")
  }
}

#if (USE_DATACACHE == 1)
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval None
  */
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id,
                                    void *private_gui_data)
{
  /* UNUSED(private_gui_data); */

  if (dc_event_id == DC_COM_NIFMAN)
  {
    dc_nifman_info_t  dc_nifman_rt_info;

    dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO,
                (void *)&dc_nifman_rt_info, sizeof(dc_nifman_rt_info));

    if (dc_nifman_rt_info.rt_state == DC_SERVICE_ON)
    {
      network_is_up = COM_SOCKETS_TRUE;
      com_socket_stat_update(COM_SOCKET_STAT_NWK_UP);
    }
    else
    {
      if (network_is_up == COM_SOCKETS_TRUE)
      {
        network_is_up = COM_SOCKETS_FALSE;
        com_socket_stat_update(COM_SOCKET_STAT_NWK_DWN);
      }
    }
  }
  else
  {
    /* Nothing to do */
  }
}
#endif /* USE_DATACACHE */

#if (USE_COM_PING == 1)
/**
  * @brief Ping response callback
  */
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp)
{
  PrintDBG("callback ping response")

  socket_msg_t   msg;
  socket_msg_t  *p_msg;
  socket_desc_t *socket_desc;

  socket_desc = com_ip_modem_find_socket(ping_socket_id, COM_SOCKETS_TRUE);
  PrintDBG("Ping rsp status:%d index:%d final:%d time:%d min:%d avg:%d max:%d",
           ping_rsp.ping_status,
           ping_rsp.index, ping_rsp.is_final_report,
           ping_rsp.time, ping_rsp.min_time, ping_rsp.avg_time, ping_rsp.max_time)

  if ((socket_desc != NULL)
      && (socket_desc->closing == COM_SOCKETS_FALSE)
      && (socket_desc->state   == SOCKET_WAITING_RSP))
  {
    if (ping_rsp.ping_status != CELLULAR_OK)
    {
      socket_desc->rsp->status = COM_SOCKETS_ERR_GENERAL;
      socket_desc->rsp->time   = 0U;
      socket_desc->rsp->bytes  = 0U;
      PrintINFO("callback ping data ready: error rcv - exit")
      msg.type = PING_MSG;
      msg.id   = DATA_RCV;
      p_msg    = &msg;
      osMessagePut(socket_desc->queue, *((uint32_t *)p_msg), 0U);
    }
    else
    {
      if (ping_rsp.index == 1U)
      {
        if (ping_rsp.is_final_report == CELLULAR_FALSE)
        {
          /* Save the data */
          socket_desc->rsp->status = COM_SOCKETS_ERR_OK;
          socket_desc->rsp->time   = ping_rsp.time;
          socket_desc->rsp->bytes  = ping_rsp.bytes;
          PrintINFO("callback ping data ready: rsp rcv - wait final report")
        }
        else
        {
          socket_desc->rsp->status = COM_SOCKETS_ERR_GENERAL;
          socket_desc->rsp->time   = 0U;
          socket_desc->rsp->bytes  = 0U;
          PrintINFO("callback ping data ready: error msg - exit")
          msg.type = PING_MSG;
          msg.id   = DATA_RCV;
          p_msg    = &msg;
          osMessagePut(socket_desc->queue, *((uint32_t *)p_msg), 0U);
        }
      }
      else
      {
        /* Must wait final report */
        if (ping_rsp.is_final_report == CELLULAR_FALSE)
        {
          PrintINFO("callback ping data ready: rsp already rcv - index %d",
                    ping_rsp.index)
        }
        else
        {
          PrintINFO("callback ping data ready: final report rcv")
          msg.type = PING_MSG;
          msg.id   = DATA_RCV;
          p_msg    = &msg;
          osMessagePut(socket_desc->queue, *((uint32_t *)p_msg), 0U);
        }
      }
    }
  }
  else
  {
    PrintINFO("!!! PURGE callback ping data ready !!!")
    if (socket_desc == NULL)
    {
      PrintERR("callback ping data ready: unknown ping")
    }
    if (socket_desc->closing == COM_SOCKETS_TRUE)
    {
      PrintERR("callback ping data ready: ping is closing")
    }
    if (socket_desc->state != SOCKET_WAITING_RSP)
    {
      PrintDBG("callback ping data ready: ping state:%d index:%d final report:%u NOK",
               socket_desc->state, ping_rsp.index, ping_rsp.is_final_report)
    }
  }
}
#endif /* USE_COM_PING */


/* Functions Definition ------------------------------------------------------*/

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
int32_t com_socket_ip_modem(int32_t family, int32_t type, int32_t protocol)
{
  int32_t sock;
  int32_t result;

  CS_IPaddrType_t IPaddrType;
  CS_TransportProtocol_t TransportProtocol;
  CS_PDN_conf_id_t PDN_conf_id;

  result = COM_SOCKETS_ERR_OK;
  sock = COM_SOCKET_INVALID_ID;

  if (family == COM_AF_INET)
  {
    /* address family IPv4 */
    IPaddrType = CS_IPAT_IPV4;
  }
  else if (family == COM_AF_INET6)
  {
    /* address family IPv6 */
    IPaddrType = CS_IPAT_IPV6;
    result = COM_SOCKETS_ERR_PARAMETER;
  }
  else
  {
    result = COM_SOCKETS_ERR_PARAMETER;
  }

  if ((type == COM_SOCK_STREAM)
      && ((protocol == COM_IPPROTO_TCP)
          || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_TCP = must be used with SOCK_STREAM */
    TransportProtocol = CS_TCP_PROTOCOL;
  }
  else if ((type == COM_SOCK_DGRAM)
           && ((protocol == COM_IPPROTO_UDP)
               || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_UDP = must be used with SOCK_DGRAM */
    TransportProtocol = CS_UDP_PROTOCOL;
    // result = COM_SOCKETS_ERR_PARAMETER;
  }
  else
  {
    result = COM_SOCKETS_ERR_PARAMETER;
  }

  PDN_conf_id = CS_PDN_CONFIG_DEFAULT;

  if (result == COM_SOCKETS_ERR_OK)
  {
    result = COM_SOCKETS_ERR_GENERAL;
    PrintDBG("socket create request")
    sock = osCDS_socket_create(IPaddrType,
                               TransportProtocol,
                               PDN_conf_id);

    if (sock != COM_SOCKET_INVALID_ID)
    {
      socket_desc_t *socket_desc;
      PrintINFO("create socket ok low level")

      /* Need to create a new socket_desc ? */
      socket_desc = com_ip_modem_provide_socket_desc(COM_SOCKETS_FALSE);
      if (socket_desc == NULL)
      {
        result = COM_SOCKETS_ERR_NOMEMORY;
        PrintERR("create socket NOK no memory")
        /* Socket descriptor is not existing in COM
           must close directly the socket and not call com_close */
        if (osCDS_socket_close(sock, 0U)
            == CELLULAR_OK)
        {
          PrintINFO("close socket ok low level")
        }
        else
        {
          PrintERR("close socket NOK low level")
        }
      }
      else
      {
        socket_desc->id    = sock;
        socket_desc->state = SOCKET_CREATED;

        if (osCDS_socket_set_callbacks(sock,
                                       com_ip_modem_data_ready_cb,
                                       NULL,
                                       com_ip_modem_closing_cb)
            == CELLULAR_OK)
        {
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PrintERR("rqt close socket issue at creation")
          if (com_closesocket_ip_modem(sock)
              == COM_SOCKETS_ERR_OK)
          {
            PrintINFO("close socket ok low level")
          }
          else
          {
            PrintERR("close socket NOK low level")
          }
        }
      }
    }
    else
    {
      PrintERR("create socket NOK low level")
    }
    /* Stat only socket whose parameters are supported */
    com_socket_stat_update((result == COM_SOCKETS_ERR_OK) ? \
                           COM_SOCKET_STAT_CRE_OK : COM_SOCKET_STAT_CRE_NOK);
  }
  else
  {
    PrintERR("create socket NOK parameter NOK")
  }

  /* result == COM_SOCKETS_ERR_OK return socket handle */
  /* result != COM_SOCKETS_ERR_OK socket not created,
     no need to call SOCKET_SET_ERROR */
  return ((result == COM_SOCKETS_ERR_OK) ? sock : result);
}

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
                                const void *optval, int32_t optlen)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if ((optval != NULL)
        && (optlen > 0))
    {
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          case COM_SO_SNDTIMEO :
          {
            if ((uint32_t)optlen <= sizeof(socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 value will not be used due to conflict risk
                 if tempo differ from low level tempo value */
              socket_desc->snd_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_RCVTIMEO :
          {
            if ((uint32_t)optlen <= sizeof(socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 if tempo value is shorter and data are received after
                 then if socket is not closing data still available in the modem
                 and can still be read if modem manage this feature
                 if tempo value is bigger and data are received before
                 then data will be send to application */
              socket_desc->rcv_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_ERROR :
          {
            /* Set for this option NOK */
            break;
          }
          default :
          {
            /* Other options NOT yet supported */
            break;
          }
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT yet supported */
      }
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

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
                                void *optval, int32_t *optlen)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if ((optval != NULL)
        && (optlen != NULL))
    {
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          case COM_SO_SNDTIMEO :
          {
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = socket_desc->snd_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_RCVTIMEO :
          {
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = socket_desc->rcv_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_ERROR :
          {
            /* Force optval to be on int32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(int32_t))
            {
              SOCKET_GET_ERROR(socket_desc, optval);
              socket_desc->error = COM_SOCKETS_ERR_OK;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          default :
          {
            /* Other options NOT yet supported */
            break;
          }
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT yet supported */
      }
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

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
                          const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result;

  socket_addr_t socket_addr;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if (socket_desc->state == SOCKET_CREATED)
    {
      if (com_translate_ip_address(addr, addrlen,
                                   &socket_addr)
          == COM_SOCKETS_TRUE)
      {
        result = COM_SOCKETS_ERR_GENERAL;
        PrintDBG("socket bind request")
        if (osCDS_socket_bind(sock,
                              socket_addr.port)
            == CELLULAR_OK)
        {
          PrintINFO("socket bind ok low level")
          result = COM_SOCKETS_ERR_OK;
          socket_desc->port = socket_addr.port;
        }
      }
      else
      {
        PrintERR("socket bind NOK translate IP NOK")
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PrintERR("socket bind NOK state invalid")
    }
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         For an opened socket as long as socket close is in error value
  *         socket must be considered as not closed and handle as not released
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_ip_modem(int32_t sock)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((socket_desc->state == SOCKET_SENDING)
        || (socket_desc->state == SOCKET_WAITING_RSP))
    {
      PrintERR("close socket NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      result = COM_SOCKETS_ERR_GENERAL;
      if (osCDS_socket_close(sock, 0U)
          == CELLULAR_OK)
      {
        com_ip_modem_delete_socket_desc(sock, COM_SOCKETS_FALSE);
        result = COM_SOCKETS_ERR_OK;
        PrintINFO("close socket ok")
      }
      else
      {
        PrintINFO("close socket NOK low level")
      }
    }
    com_socket_stat_update((result == COM_SOCKETS_ERR_OK) ? \
                           COM_SOCKET_STAT_CLS_OK : COM_SOCKET_STAT_CLS_NOK);
  }

  return (result);
}


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
                             const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result;
  socket_addr_t socket_addr;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;

  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if (socket_desc->state == SOCKET_CREATED)
    {
      if (com_translate_ip_address(addr, addrlen,
                                   &socket_addr)
          == COM_SOCKETS_TRUE)
      {
        /* Check Network status */
        if (com_ip_modem_is_network_up() == COM_SOCKETS_TRUE)
        {
          if (osCDS_socket_connect(socket_desc->id,
                                   socket_addr.ip_type,
                                   &socket_addr.ip_value[0],
                                   socket_addr.port)
              == CELLULAR_OK)
          {
            result = COM_SOCKETS_ERR_OK;
            PrintINFO("socket connect ok")
            socket_desc->state = SOCKET_CONNECTED;
          }
          else
          {
            result = COM_SOCKETS_ERR_GENERAL;
            PrintERR("socket connect NOK at low level")
          }
        }
        else
        {
          result = COM_SOCKETS_ERR_NONETWORK;
          PrintERR("socket connect NOK no network")
        }
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PrintERR("socket connect NOK state invalid")
    }

    com_socket_stat_update((result == COM_SOCKETS_ERR_OK) ? \
                           COM_SOCKET_STAT_CNT_OK : COM_SOCKET_STAT_CNT_NOK);
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}


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
                          int32_t flags)
{
  com_bool_t is_network_up;
  socket_desc_t *socket_desc;
  int32_t result;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);
  is_network_up = COM_SOCKETS_FALSE;

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    if (socket_desc->state == SOCKET_CONNECTED)
    {
      /* closing maybe received, refuse to send data */
      if (socket_desc->closing == COM_SOCKETS_FALSE)
      {
        /* network maybe down, refuse to send data */
        if (com_ip_modem_is_network_up() == COM_SOCKETS_FALSE)
        {
          result = COM_SOCKETS_ERR_NONETWORK;
          PrintERR("snd data NOK no network")
        }
        else
        {
          uint32_t length_to_send;
          uint32_t length_send;

          result = COM_SOCKETS_ERR_GENERAL;
          length_send = 0U;
          socket_desc->state = SOCKET_SENDING;

          if (flags == COM_MSG_DONTWAIT)
          {
            length_to_send = COM_MIN((uint32_t)len, MODEM_MAX_TX_DATA_SIZE);
            if (osCDS_socket_send(socket_desc->id,
                                  buf, length_to_send)
                == CELLULAR_OK)
            {
              length_send = length_to_send;
              result = (int32_t)length_send;
              PrintINFO("snd data DONTWAIT ok")
            }
            else
            {
              PrintERR("snd data DONTWAIT NOK at low level")
            }
            socket_desc->state = SOCKET_CONNECTED;
          }
          else
          {
            is_network_up = com_ip_modem_is_network_up();
            /* Send all data of a big buffer - Whatever the size */
            while ((length_send != (uint32_t)len)
                   && (socket_desc->closing == COM_SOCKETS_FALSE)
                   && (is_network_up == COM_SOCKETS_TRUE)
                   && (socket_desc->state == SOCKET_SENDING))
            {
              length_to_send = COM_MIN(((uint32_t)len) - length_send,
                                       MODEM_MAX_TX_DATA_SIZE);
              /* A tempo is already managed at low-level */
              if (osCDS_socket_send(socket_desc->id,
                                    buf + length_send,
                                    length_to_send)
                  == CELLULAR_OK)
              {
                length_send += length_to_send;
                PrintINFO("snd data ok")
                /* Update Network status */
                is_network_up = com_ip_modem_is_network_up();
              }
              else
              {
                socket_desc->state = SOCKET_CONNECTED;
                PrintERR("snd data NOK at low level")
              }
            }
            socket_desc->state = SOCKET_CONNECTED;
            result = (int32_t)length_send;
          }
        }
      }
      else
      {
        PrintERR("snd data NOK socket closing")
        result = COM_SOCKETS_ERR_CLOSING;
      }
    }
    else
    {
      PrintERR("snd data NOK err state")
      if (socket_desc->state < SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (socket_desc->state == SOCKET_CLOSING) ? \
                 COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    com_socket_stat_update((result >= 0) ? \
                           COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);
  }

  if (result >= 0)
  {
    SOCKET_SET_ERROR(socket_desc, COM_SOCKETS_ERR_OK);
  }
  else
  {
    SOCKET_SET_ERROR(socket_desc, result);
  }

  return (result);
}

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
                          int32_t flags)
{
  int32_t result;
  int32_t len_rcv;
  osEvent event;
  socket_desc_t *socket_desc;
  socket_msg_t   msg;
  socket_msg_t  *p_msg;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock, COM_SOCKETS_FALSE);

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    /* Closing maybe received or Network maybe done
       but still some data to read */
    if (socket_desc->state == SOCKET_CONNECTED)
    {
      uint32_t length_to_read;
      length_to_read = COM_MIN((uint32_t)len, MODEM_MAX_RX_DATA_SIZE);
      result = COM_SOCKETS_ERR_GENERAL;
      socket_desc->state = SOCKET_WAITING_RSP;

      if (flags == COM_MSG_DONTWAIT)
      {
        /* Application don't want to wait if there is no data available */
        len_rcv = osCDS_socket_receive(socket_desc->id,
                                       buf, length_to_read);
        result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
        socket_desc->state = SOCKET_CONNECTED;
        PrintINFO("rcv data DONTWAIT")
      }
      else
      {
        /* Maybe still some data available
           because application don't read all data with previous calls */
        PrintDBG("rcv data waiting")
        len_rcv = osCDS_socket_receive(socket_desc->id,
                                       buf, length_to_read);
        PrintDBG("rcv data waiting exit")

        if (len_rcv == 0)
        {
          /* Waiting for Distant response or Closure Socket or Timeout */
          event = osMessageGet(socket_desc->queue,
                               socket_desc->rcv_timeout);
          if (event.status == osEventTimeout)
          {
            result = COM_SOCKETS_ERR_TIMEOUT;
            socket_desc->state = SOCKET_CONNECTED;
            PrintINFO("rcv data exit timeout")
          }
          else
          {
            p_msg = (socket_msg_t *)(&(event.value.v));
            msg   = *p_msg;
            if (msg.type == SOCKET_MSG)
            {
              switch (msg.id)
              {
                case DATA_RCV :
                {
                  len_rcv = osCDS_socket_receive(socket_desc->id,
                                                 buf, (uint32_t)len);
                  result = (len_rcv < 0) ? \
                           COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
                  socket_desc->state = SOCKET_CONNECTED;
                  PrintINFO("rcv data exit with data")
                  break;
                }
                case CLOSING_RCV :
                {
                  result = COM_SOCKETS_ERR_CLOSING;
                  socket_desc->state = SOCKET_CLOSING;
                  PrintINFO("rcv data exit socket closing")
                  break;
                }
                default :
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  socket_desc->state = SOCKET_CONNECTED;
                  PrintERR("rcv data exit NOK impossible case")
                  break;
                }
              }
            }
            else
            {
              /* Impossible case */
              result = COM_SOCKETS_ERR_GENERAL;
              socket_desc->state = SOCKET_CONNECTED;
              PrintERR("rcv ping msg NOK impossible case")
            }
          }
        }
        else
        {
          result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
          socket_desc->state = SOCKET_CONNECTED;
          PrintINFO("rcv data exit data available or err low level")
        }
      }
    }
    else
    {
      PrintERR("recv data NOK err state")
      if (socket_desc->state < SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (socket_desc->state == SOCKET_CLOSING) ? \
                 COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    com_socket_stat_update((result == COM_SOCKETS_ERR_OK) ? \
                           COM_SOCKET_STAT_RCV_OK : COM_SOCKET_STAT_RCV_NOK);
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return ((result == COM_SOCKETS_ERR_OK) ? len_rcv : result);
}


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
                            int32_t backlog)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

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
                            com_sockaddr_t *addr, int32_t *addrlen)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

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
                            const com_sockaddr_t *to, int32_t tolen)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

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
                              com_sockaddr_t *from, int32_t *fromlen)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}


/*** Other functionalities ****************************************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  None
  * @retval bool      - true/false init ok/nok
  */
com_bool_t com_init_ip_modem(void)
{
  com_bool_t result;

  result = COM_SOCKETS_FALSE;

#if (USE_COM_PING == 1)
  ping_socket_id = COM_SOCKET_INVALID_ID;
#endif

  for (uint8_t i = 0U; i < COM_SOCKET_LOCAL_ID_NB; i++)
  {
    socket_local_id[i] = COM_SOCKETS_FALSE; /* set socket local id to unused */
  }
  osMutexDef(ComSocketsMutex);
  ComSocketsMutexHandle = osMutexCreate(osMutex(ComSocketsMutex));
  if (ComSocketsMutexHandle != NULL)
  {
    /* Create always the first element of the list */
    socket_desc_list = com_ip_modem_create_socket_desc();
    if (socket_desc_list != NULL)
    {
      socket_desc_list->next = NULL;
      socket_desc_list->state = SOCKET_INVALID;
      result = COM_SOCKETS_TRUE;
    }

    com_socket_stat_update(COM_SOCKET_STAT_INIT);

#if (USE_DATACACHE == 1)
    network_is_up = COM_SOCKETS_FALSE;
#endif
  }

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
void com_start_ip_modem(void)
{
#if (USE_DATACACHE == 1)
  /* Datacache registration for netwok on/off status */
  dc_com_register_gen_event_cb(&dc_com_db,
                               com_socket_datacache_cb,
                               (void *) NULL);
#endif
}


/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         DNS resolver is a fix value in the module
  *         only a primary DNS is used see com_primary_dns_addr_str value
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @note   only IPv4 address is managed
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_ip_modem(const com_char_t *name,
                                   com_sockaddr_t   *addr)
{
  int32_t result;
  CS_PDN_conf_id_t PDN_conf_id;
  CS_DnsReq_t  dns_req;
  CS_DnsResp_t dns_resp;

  PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
  result = COM_SOCKETS_ERR_PARAMETER;

  if ((name != NULL)
      && (addr != NULL))
  {
    if (strlen((const char *)name) <= sizeof(dns_req.host_name))
    {
      strcpy((char *)&dns_req.host_name[0],
             (const char *)name);

      result = COM_SOCKETS_ERR_GENERAL;
      if (osCDS_dns_request(PDN_conf_id,
                            &dns_req,
                            &dns_resp)
          == CELLULAR_OK)
      {
        PrintINFO("DNS resolution OK: %s %s", name, dns_resp.host_addr)
        if (com_convert_IPString_to_sockaddr((com_char_t *)&dns_resp.host_addr[0],
                                             addr)
            == COM_SOCKETS_TRUE)
        {
          PrintDBG("DNS conversion OK")
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PrintERR("DNS conversion NOK")
        }
      }
      else
      {
        PrintERR("DNS resolution NOK for %s", name)
      }
    }
  }

  return (result);
}

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
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

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
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  None
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_ip_modem(void)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;

  /* Need to create a new socket_desc ? */
  socket_desc = com_ip_modem_provide_socket_desc(COM_SOCKETS_TRUE);
  if (socket_desc == NULL)
  {
    result = COM_SOCKETS_ERR_NOMEMORY;
    PrintERR("create ping NOK no memory")
    /* Socket descriptor is not existing in COM
       and nothing to do at low level */
  }
  else
  {
    /* Socket state is set directly to CREATED
       because nothing else as to be done */
    result = COM_SOCKETS_ERR_OK;
    socket_desc->state = SOCKET_CREATED;
  }

  return ((result == COM_SOCKETS_ERR_OK) ? socket_desc->id : result);
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
int32_t com_ping_process_ip_modem(int32_t ping,
                                  const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp)
{
  int32_t  result;
  uint32_t timeout_ms;
  socket_desc_t *socket_desc;
  socket_addr_t  socket_addr;
  CS_Ping_params_t ping_params;
  CS_PDN_conf_id_t PDN_conf_id;
  osEvent event;
  socket_msg_t  msg;
  socket_msg_t *p_msg;

  result = COM_SOCKETS_ERR_PARAMETER;

  socket_desc = com_ip_modem_find_socket(ping, COM_SOCKETS_TRUE);

  if (socket_desc != NULL)
  {
    /* No need each time to close the connection */
    if ((socket_desc->state == SOCKET_CREATED)
        || (socket_desc->state == SOCKET_CONNECTED))
    {
      if ((timeout != 0U)
          && (rsp != NULL)
          && (addr != NULL))
      {
        if (com_translate_ip_address(addr, addrlen,
                                     &socket_addr)
            == COM_SOCKETS_TRUE)
        {
          /* Check Network status */
          if (com_ip_modem_is_network_up() == COM_SOCKETS_TRUE)
          {
            PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
            ping_params.timeout = timeout;
            ping_params.pingnum = 1U;
            strcpy((char *)&ping_params.host_addr[0],
                   (char *)&socket_addr.ip_value[0]);
            socket_desc->state = SOCKET_WAITING_RSP;
            socket_desc->rsp   =  rsp;
            if (osCDS_ping(PDN_conf_id,
                           &ping_params,
                           com_ip_modem_ping_rsp_cb)
                == CELLULAR_OK)
            {
              result = COM_SOCKETS_ERR_OK;
              ping_socket_id = socket_desc->id;
              timeout_ms = (uint32_t)timeout * 1000U;
              /* Waiting for Response or Timeout */
              event = osMessageGet(socket_desc->queue,
                                   timeout_ms);
              if (event.status == osEventTimeout)
              {
                result = COM_SOCKETS_ERR_TIMEOUT;
                socket_desc->state = SOCKET_CONNECTED;
                PrintINFO("ping exit timeout")
              }
              else
              {
                p_msg = (socket_msg_t *)(&(event.value.v));
                msg   = *p_msg;
                if (msg.type == PING_MSG)
                {
                  switch (msg.id)
                  {
                    case DATA_RCV :
                    {
                      result = COM_SOCKETS_ERR_OK;
                      /* Format the data */
                      socket_desc->state = SOCKET_CONNECTED;
                      break;
                    }
                    case CLOSING_RCV :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      socket_desc->state = SOCKET_CONNECTED;
                      PrintERR("rcv data exit NOK closing case")
                      break;
                    }
                    default :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      socket_desc->state = SOCKET_CONNECTED;
                      PrintERR("rcv data exit NOK impossible case")
                      break;
                    }
                  }
                }
                else
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  socket_desc->state = SOCKET_CONNECTED;
                  PrintERR("rcv socket msg NOK impossible case")
                }
              }
            }
            else
            {
              socket_desc->state = SOCKET_CONNECTED;
              result = COM_SOCKETS_ERR_GENERAL;
              PrintERR("ping send NOK at low level")
            }
          }
          else
          {
            result = COM_SOCKETS_ERR_NONETWORK;
            PrintERR("ping send NOK no network")
          }
        }
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PrintERR("ping send NOK state invalid")
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
int32_t com_closeping_ip_modem(int32_t ping)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(ping, COM_SOCKETS_TRUE);

  if (socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((socket_desc->state == SOCKET_SENDING)
        || (socket_desc->state == SOCKET_WAITING_RSP))
    {
      PrintERR("close ping NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      result = COM_SOCKETS_ERR_GENERAL;
      com_ip_modem_delete_socket_desc(ping, COM_SOCKETS_TRUE);
      result = COM_SOCKETS_ERR_OK;
      PrintINFO("close ping ok")
    }
  }

  return result;
}

#endif /* USE_COM_PING */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
