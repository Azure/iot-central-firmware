/**
  ******************************************************************************
  * @file    cellular_service_int.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service.c module
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
#ifndef CELLULAR_SERVICE_INT_H
#define CELLULAR_SERVICE_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cellular_service.h"

/* Exported constants --------------------------------------------------------*/
#define MAX_PINCODE_SIZE               (16U)
#define MAX_APN_SIZE                   (64U)
#define MAX_IP_ADDR_SIZE               (64U)

#define DEFAULT_IP_MAX_PACKET_SIZE     (1500U) /* TODO: Hard-Coded but should use real modem limit */
#define DEFAULT_TRP_MAX_TIMEOUT        (90U)
#define DEFAULT_TRP_CONN_SETUP_TIMEOUT (600U)
#define DEFAULT_TRP_TRANSFER_TIMEOUT   (50U)
#define DEFAULT_TRP_SUSPEND_TIMEOUT    (1000U)
#define DEFAULT_TRP_RX_TIMEOUT         (50U)

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  CSMT_UNDEFINED = 3,
  CSMT_NONE,

  /* URC notifications */
  CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS,   /* for +CEREG */
  CSMT_URC_EPS_LOCATION_INFO,                 /* for +CEREG */
  CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS,  /* for +CGREG */
  CSMT_URC_GPRS_LOCATION_INFO,                /* for +CGREG */
  CSMT_URC_CS_NETWORK_REGISTRATION_STATUS,    /* for +CREG */
  CSMT_URC_CS_LOCATION_INFO,                  /* for +CREG */
  CSMT_URC_SIGNAL_QUALITY,                    /* not in rec, modem dependant */
  CSMT_URC_PACKET_DOMAIN_EVENT,               /* for +CGEV */
  CSMT_URC_SOCKET_DATA_PENDING,               /* for socket data available */
  CSMT_URC_SOCKET_CLOSED,                     /* for socket closed by remote */
  CSMT_URC_MODEM_EVENT,                       /* for subscribed modem events */
  CSMT_URC_PING_RSP,                          /* for ping responses */
  /* internal messages and corresponding structure */
  CSMT_PINCODE,            /* csint_pinCode_t */
  CSMT_INITMODEM,          /* csint_modemInit_t */
  CSMT_ATTACHSTATUS,       /* CS_PSattach_t */
  CSMT_OPERATORSELECT,     /* CS_OperatorSelector_t */
  CSMT_REGISTRATIONSTATUS, /* CS_RegistrationStatus_t */
  CSMT_URC_EVENT,          /* CS_UrcEvent_t */
  CSMT_DEVICE_INFO,        /* CS_DeviceInfo_t */
  CSMT_SOCKET_INFO,        /* csint_socket_infos_t */
  CSMT_SOCKET_DATA_BUFFER, /* csint_socket_data_buffer_t */
  CSMT_SOCKET_RXDATA,      /* uint32_t (size) */
  CSMT_SOCKET_CNX_STATUS,  /* csint_socket_cnx_infos_t */
  CSMT_SIGNAL_QUALITY,     /* csint_signal_quality_t */
  CSMT_ATTACH_PS_DOMAIN,   /* uint32_t */
  CSMT_DETACH_PS_DOMAIN,   /* uint32_t */
  CSMT_RESET,              /* CS_Reset_t */
  CSMT_ACTIVATE_PDN,       /* CS_PDN_conf_id_t */
  CSMT_DEACTIVATE_PDN,     /* CS_PDN_conf_id_t */
  CSMT_DEFINE_PDN,         /* csint_pdn_infos_t */
  CSMT_SET_DEFAULT_PDN,    /* CS_PDN_conf_id_t */
  CSMT_GET_IP_ADDRESS,     /* CS_PDN_conf_id_t */
  CSMT_MODEMCONFIG,        /* not implemented yet */
  CSMT_DNS_REQ,            /* csint_dns_req_t */
  CSMT_PING_ADDRESS,       /* csint_ping_params_t */
  CSMT_MODEM_EVENT,        /* CS_ModemEvent_t */
  CSMT_ERROR_REPORT,       /* csint_error_report_t */

} csint_msgType_t;

typedef struct
{
  CS_CHAR_t pincode[MAX_PINCODE_SIZE];
} csint_pinCode_t;

typedef struct
{
  CS_ModemInit_t      init;
  CS_Bool_t           reset;
  csint_pinCode_t     pincode;
} csint_modemInit_t;

typedef struct
{
  uint16_t ci;
  uint8_t  lac;
  CS_Bool_t ci_updated;
  CS_Bool_t lac_updated;
} csint_location_info_t;

typedef enum
{
  CGEV_EVENT_UNDEFINE          = 0,
  /* EVENT ORIGINE */
  CGEV_EVENT_ORIGINE_NW        = 1,
  CGEV_EVENT_ORIGINE_ME        = 2,
  /* EVENT SCOPE */
  CGEV_EVENT_SCOPE_PDN         = 10,
  CGEV_EVENT_SCOPE_GLOBAL      = 11,
  /* EVENT TYPE */
  CGEV_EVENT_TYPE_ACTIVATION   = 12,
  CGEV_EVENT_TYPE_DEACTIVATION = 13,
  CGEV_EVENT_TYPE_REJECT       = 14,
  CGEV_EVENT_TYPE_DETACH       = 15,
  CGEV_EVENT_TYPE_CLASS        = 16,
  CGEV_EVENT_TYPE_MODIFY       = 17,
} csint_CGEV_event_values_t;

typedef struct
{
  csint_CGEV_event_values_t   event_origine;
  csint_CGEV_event_values_t   event_scope;
  csint_CGEV_event_values_t   event_type;
  CS_PDN_conf_id_t            conf_id;      /* PDN on which event occured
                                               * CS_PDN_NOT_DEFINED if not known
                                               * CS_PDN_ALL if all PDN impacte
                                               */
} csint_PDN_event_desc_t;

typedef struct
{
  CS_Bool_t eps_network_registration;
  CS_Bool_t gprs_network_registration;
  CS_Bool_t cs_network_registration;
  CS_Bool_t eps_location_info;
  CS_Bool_t gprs_location_info;
  CS_Bool_t cs_location_info;
  CS_Bool_t signal_quality;
  CS_Bool_t packet_domain_event;
  CS_Bool_t ping_rsp;
} csint_urc_subscription_t;

typedef struct
{
  CS_PDN_conf_id_t        conf_id;
  CS_CHAR_t               apn[MAX_APN_SIZE];
  CS_PDN_configuration_t  pdn_conf;
} csint_pdn_infos_t;

typedef struct
{
  CS_PDN_conf_id_t    conf_id;
  CS_DnsConf_t        dns_conf;
  CS_DnsReq_t         dns_req;
} csint_dns_request_t;

typedef struct
{
  CS_PDN_conf_id_t    conf_id;
  CS_Ping_params_t    ping_params;
} csint_ping_params_t;

typedef enum
{
  SOCKETSTATE_NOT_ALLOC         = 0,
  SOCKETSTATE_CREATED           = 1,
  SOCKETSTATE_CONNECTED         = 2,
  SOCKETSTATE_ALLOC_BUT_INVALID = 3, /* valid from client point of view but not from modem (exple: after reset) */
} csint_Socket_State_t;

typedef struct
{
  /* socket identifier */
  socket_handle_t    socket_handle;

  /* socket states */
  csint_Socket_State_t state; /* socket state */

  CS_SocketOptionName_t config;

  /* parameters set during socket creation */
  CS_IPaddrType_t             addr_type;   /* local IP address */
  CS_TransportProtocol_t      protocol;
  uint16_t                    local_port;
  CS_PDN_conf_id_t            conf_id;     /* PDP context identifier used for this transfer */

  /* parameters set during socket connect */
  CS_IPaddrType_t     ip_addr_type;
  CS_CHAR_t           ip_addr_value[MAX_IP_ADDR_SIZE]; /* remote IP address */
  uint16_t            remote_port;

  /* parameters set during socket configuration */
  uint16_t ip_max_packet_size; /* 0 to 1500 bytes, 0 means default, default = DEFAULT_IP_MAX_PACKET_SIZE */
  uint16_t trp_max_timeout; /* 0 to 65535 seconds, 0 means infinite, default = DEFAULT_TRP_MAX_TIMEOUT */
  uint16_t trp_conn_setup_timeout; /* 10 to 1200 hundreds of ms, 0 means infinite, default = DEFAULT_TRP_CONN_SETUP_TIMEOUT */
  uint8_t  trp_transfer_timeout; /* 0 to 255 ms, 0 means infinite, default = DEFAULT_TRP_TRANSFER_TIMEOUT */
  CS_ConnectionMode_t trp_connect_mode;
  uint16_t trp_suspend_timeout; /* 0 to 2000 ms , 0 means infinite, default = DEFAULT_TRP_SUSPEND_TIMEOUT */
  uint8_t  trp_rx_timeout; /* 0 to 255 ms, 0 means infinite, default = DEFAULT_TRP_RX_TIMEOUT */

  /* socket infos callbacks */
  cellular_socket_data_ready_callback_t   socket_data_ready_callback;
  cellular_socket_data_ready_callback_t   socket_data_sent_callback;
  cellular_socket_closed_callback_t       socket_remote_close_callback;

} csint_socket_infos_t;

typedef struct
{
  socket_handle_t  socket_handle;
  const CS_CHAR_t  *p_buffer_addr_send; /* send buffer (const) */
  CS_CHAR_t        *p_buffer_addr_rcv;  /* receive buffer */
  uint32_t         buffer_size;         /* real buffer size */
  uint32_t         max_buffer_size;     /* maximum buffer size allowed by client - used on for RX data */
} csint_socket_data_buffer_t;

typedef struct
{
  CS_IPaddrType_t     ip_addr_type;
  CS_CHAR_t           ip_addr_value[MAX_IP_ADDR_SIZE];
} csint_ip_addr_info_t;

typedef struct
{
  socket_handle_t      socket_handle;
  CS_SocketCnxInfos_t  *infos;
} csint_socket_cnx_infos_t;


typedef enum
{
  CSERR_UNKNOWN    = 0,
  CSERR_SIM        = 1,

} csint_error_type_t;

typedef enum
{
  CS_SIMSTATE_UNKNOWN,
  CS_SIMSTATE_READY,
  CS_SIMSTATE_SIM_NOT_INSERTED,
  CS_SIMSTATE_SIM_BUSY,
  CS_SIMSTATE_SIM_FAILURE,
  CS_SIMSTATE_SIM_WRONG,
  CS_SIMSTATE_SIM_PIN_REQUIRED,
  CS_SIMSTATE_SIM_PIN2_REQUIRED,
  CS_SIMSTATE_SIM_PUK_REQUIRED,
  CS_SIMSTATE_SIM_PUK2_REQUIRED,
  CS_SIMSTATE_INCORRECT_PASSWORD,
} csint_SIMState_t;

typedef struct
{
  csint_error_type_t  error_type;

  /* detailled error infos =f(error_type) are listed below */
  csint_SIMState_t    sim_state; /* if error_type = CSERR_SIM */

} csint_error_report_t;

/* List of Cellular Service IDs */
typedef enum
{
  SID_CS_CHECK_CNX = CELLULAR_SERVICE_START_ID,
  /* Control */
  SID_CS_POWER_ON,
  SID_CS_POWER_OFF,
  SID_CS_INIT_MODEM,
  SID_CS_GET_DEVICE_INFO,
  SID_CS_REGISTER_NET,
  SID_CS_SUSBCRIBE_NET_EVENT,
  SID_CS_UNSUSBCRIBE_NET_EVENT,
  SID_CS_GET_NETSTATUS,
  SID_CS_GET_ATTACHSTATUS,
  SID_CS_GET_SIGNAL_QUALITY,
  SID_CS_ACTIVATE_PDN,
  SID_ATTACH_PS_DOMAIN,
  SID_DETACH_PS_DOMAIN,
  SID_CS_DEACTIVATE_PDN,
  SID_CS_REGISTER_PDN_EVENT,
  SID_CS_DEREGISTER_PDN_EVENT,
  SID_CS_GET_IP_ADDRESS,
  SID_CS_DEFINE_PDN,
  SID_CS_SET_DEFAULT_PDN,
  /* SID_CS_AUTOACTIVATE_PDN, */
  /* SID_CS_CONNECT, */
  /* SID_CS_DISCONNECT, */
  SID_CS_DIAL_ONLINE, /* NOT SUPPORTED */
  SID_CS_DIAL_COMMAND,
  SID_CS_SEND_DATA,
  SID_CS_RECEIVE_DATA,
  /* SID_CS_SOCKET_CREATE, */     /* not needed, config is done at CS level */
  /* SID_CS_SOCKET_SET_OPTION, */ /* not needed, config is done at CS level */
  /* SID_CS_SOCKET_GET_OPTION, */ /* not needed, config is done at CS level */
  /* SID_CS_SOCKET_RECEIVE_DATA, */
  SID_CS_SOCKET_CLOSE,
  /* DATA mode and COMMAND mode swiches */
  SID_CS_DATA_SUSPEND,
  SID_CS_DATA_RESUME,
  SID_CS_RESET,
  SID_CS_SOCKET_CNX_STATUS,
  SID_CS_MODEM_CONFIG,
  SID_CS_DNS_REQ,
  SID_CS_PING_IP_ADDRESS,
  SID_CS_SUSBCRIBE_MODEM_EVENT,

} CS_msg_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_INT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

