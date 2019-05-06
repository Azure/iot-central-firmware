/**
  ******************************************************************************
  * @file    at_custom_modem.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem.c module
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
#ifndef AT_CUSTOM_MODEM_H
#define AT_CUSTOM_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_custom_common.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
#define MODEM_DEFAULT_TIMEOUT      ((uint32_t) 10000U)
#define MODEM_PDP_MAX_TYPE_SIZE    ((uint32_t) 8U)
#define MODEM_PDP_MAX_APN_SIZE     ((uint32_t) 64U)
#define MODEM_MAX_NB_PDP_CTXT      ((uint8_t) CS_PDN_CONFIG_MAX + 1U) /* max. nbr of local PDP context configs */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  /* standard commands */
  _AT = 0, /* empty command or empty answer */
  _AT_OK,
  _AT_CONNECT,
  _AT_RING,
  _AT_NO_CARRIER,
  _AT_ERROR,
  _AT_NO_DIALTONE,
  _AT_BUSY,
  _AT_NO_ANSWER,
  _AT_CME_ERROR,
  _AT_CMS_ERROR,

  /* 3GPP TS 27.007 and GSM 07.07 commands */
  _AT_CGMI, /* Request manufacturer identification */
  _AT_CGMM, /* Model identification */
  _AT_CGMR, /* Revision identification */
  _AT_CGSN, /* Product serial number identification */
  _AT_CIMI, /* IMSI */
  _AT_CEER, /* Extended error report */
  _AT_CMEE, /* Report mobile equipment error */
  _AT_CPIN, /* Enter PIN */
  _AT_CFUN, /* Set phone functionality */
  _AT_COPS, /* Operator selection */
  _AT_CNUM, /* Subscriber number */
  _AT_CGATT,   /* PS attach or detach */
  _AT_CREG,    /* Network registration: enable or disable +CREG urc */
  _AT_CGREG,   /* GPRS network registation status: enable or disable +CGREG urc */
  _AT_CEREG,   /* EPS network registration status: enable or disable +CEREG urc */
  _AT_CGEREP,  /* Packet domain event reporting: enable or disable +CGEV urc */
  _AT_CGEV,    /* EPS bearer indication status */
  _AT_CSQ,     /* Signal quality */
  _AT_CGDCONT, /* Define PDP context */
  _AT_CGACT,   /* PDP context activate or deactivate */
  _AT_CGDATA,  /* Enter Data state */
  _AT_CGPADDR, /* Show PDP Address */

  /* V.25TER commands */
  _ATD,       /* Dial */
  _ATE,       /* Command Echo */
  _ATH,       /* Hook control (disconnect existing connection) */
  _ATO,       /* Return to online data state (switch from COMMAND to DATA mode) */
  _ATV,       /* DCE response format */
  _AT_AND_W,  /* Store current Parameters to User defined profile */
  _AT_AND_D,  /* Set DTR function mode */
  _ATX,       /* CONNECT Result code and monitor call progress */
  _AT_GSN,    /* Request product serial number identification */
  _AT_IPR,    /* Fixed DTE rate */
  _AT_IFC,    /* set DTE-DCE local flow control */

  /* other */
  _AT_ESC_CMD, /* escape command for switching from DATA to COMMAND mode */

  _AT_LAST_GENERIC, /* keep it at last position */

} atcustom_modem_standard_cmd_t;

typedef enum
{
  /*  WAITING FOR  /  COMMAND FORMAT */
  WAITING_FOR_INIT_CR,       /*   <CR>        /  xxxxx<CR> */
  WAITING_FOR_FIRST_CHAR,    /*   <CR> or x   /                     */
  WAITING_FOR_CR,            /*   <CR>        /                     */
  WAITING_FOR_LF,            /*   <LF>        /                     */
  WAITING_FOR_SOCKET_DATA,   /* count received characters, do not analyze them */
} atcustom_modem_SyntaxAutomatonState_t;

typedef enum
{
  CMEE_DISABLE = 0,
  CMEE_NUMERIC = 1,
  CMEE_VERBOSE = 2,
} atcustom_CMEE_n_t;

typedef enum
{
  CGATT_DETACHED = 0,
  CGATT_ATTACHED = 1,
  CGATT_UNKNOWN  = 2, /* cggatt state not set */
} atcustom_CGATT_state_t;

/* CxREG: common values for CREG, CGREG or CEREG */
typedef enum
{
  CXREG_DISABLE_NETWK_REG_URC        = 0,
  CXREG_ENABLE_NETWK_REG_URC         = 1,
  CXREG_ENABLE_NETWK_REG_LOC_URC     = 2,
  CXREG_ENABLE_NETWK_REG_LOC_EMM_URC = 3,
} atcustom_CxREG_n_t;

typedef enum
{
  CGSN_SN     = 0,
  CGSN_IMEI   = 1,
  CGSN_IMEISV = 2,
  CGSN_SVN    = 3,
} atcustom_CGSN_snt_t;

typedef struct
{
  uint8_t               cid_value;        /* cid value sent to the modem in CGDCONT, CGACT, CGATT... commands */
  at_bool_t             pdn_defined;      /* is this cid value already defined by user ? TRUE if yes, FALSE is no */
  CS_PDN_conf_id_t      affected_config;  /* PDN config using this cid */
  csint_ip_addr_info_t  ip_addr_infos;    /* IP address associated to this PDN */
} atcustom_modem_cid_table_t;

typedef enum
{
  SocketSendState_No_Activity,
  SocketSendState_WaitingPrompt1st_greaterthan, /* waiting for '>' */
  SocketSendState_WaitingPrompt2nd_space, /* waiting for ' ' */
  SocketSendState_Prompt_Received,
} atcustom_socket_send_state_t;

typedef enum
{
  SocketRcvState_No_Activity,
  SocketRcvState_RequestSize,          /* send command to know socket data size to read */
  SocketRcvState_RequestData_Header,   /* send command to read socket data: 1st step = read header */
  SocketRcvState_RequestData_Payload,  /* send command to read socket data: 1st step = read payload */
} atcustom_socket_receive_state_t;

typedef enum
{
  SocketRxDataState_not_started,
  SocketRxDataState_waiting_header,
  SocketRxDataState_receiving_header,
  SocketRxDataState_receiving_data,
  SocketRxDataState_data_received,
  SocketRxDataState_finished,
} atcustom_socket_RxData_state_t;

typedef struct
{
  uint8_t     socket_connId_value;        /* modem value corresponding to this index */
  at_bool_t   socket_connected;           /* is socket connected ? */
  at_bool_t   socket_data_pending_urc;    /* is there a pending data urc for this connID ? */
  at_bool_t   socket_closed_pending_urc;  /* is there a pending closed urc for this connID ?*/
} atcustom_persistent_SOCKET_context_t;

/* atcustom_persistent_context_t is a structure to save datas
*  which are persistent across different SID
*/
typedef struct
{
  /* DATA mode indication: if true, modem is in DATA modem / if false, modem is in command mode (default) */
  at_bool_t   modem_in_data_mode;

  /* URC subscriptions */
  CS_Bool_t   urc_subscript_eps_networkReg;
  CS_Bool_t   urc_subscript_eps_locationInfo;
  CS_Bool_t   urc_subscript_gprs_networkReg;
  CS_Bool_t   urc_subscript_gprs_locationInfo;
  CS_Bool_t   urc_subscript_cs_networkReg;
  CS_Bool_t   urc_subscript_cs_locationInfo;
  CS_Bool_t   urc_subscript_signalQuality;
  CS_Bool_t   urc_subscript_pdn_event;

  /* URC received */
  at_bool_t   urc_avail_eps_network_registration;
  at_bool_t   urc_avail_eps_location_info_tac;
  at_bool_t   urc_avail_eps_location_info_ci;
  at_bool_t   urc_avail_gprs_network_registration;
  at_bool_t   urc_avail_gprs_location_info_lac;
  at_bool_t   urc_avail_gprs_location_info_ci;
  at_bool_t   urc_avail_cs_network_registration;
  at_bool_t   urc_avail_cs_location_info_lac;
  at_bool_t   urc_avail_cs_location_info_ci;
  at_bool_t   urc_avail_signal_quality;
  at_bool_t   urc_avail_socket_data_pending;
  at_bool_t   urc_avail_socket_closed_by_remote;
  at_bool_t   urc_avail_pdn_event;

  /* Modem events subscriptions */
  CS_ModemEvent_t modem_events_subscript; /* bitmask */

  /* Modem events received */
  CS_ModemEvent_t urc_avail_modem_events; /* bitmask */

  /* URC datas */
  csint_location_info_t  eps_location_info;  /* updated on CEREG reception (answer to CEREG cmd or URC) */
  CS_NetworkRegState_t   eps_network_state;  /* updated on CEREG reception (answer to CEREG cmd or URC) */
  csint_location_info_t  gprs_location_info; /* updated on CGREG reception (answer to CGREG cmd or URC) */
  CS_NetworkRegState_t   gprs_network_state; /* updated on CGREG reception (answer to CGREG cmd or URC) */
  csint_location_info_t  cs_location_info;   /* updated on CREG reception (answer to CREG cmd or URC) */
  CS_NetworkRegState_t   cs_network_state;   /* updated on CREG reception (answer to CREG cmd or URC) */
  CS_SignalQuality_t     signal_quality;     /* updated on signal quality URC (modem specific) */

  /* PDN event URC */
  csint_PDN_event_desc_t  pdn_event;     /* updated on CGEV reception (URC received for CGEREP) */

  /* PDP context infos */
  csint_pdn_infos_t           pdp_ctxt_infos[MODEM_MAX_NB_PDP_CTXT];      /* table of PDP context */
  atcustom_modem_cid_table_t  modem_cid_table[MODEM_MAX_NB_PDP_CTXT];     /* allocation table of modem cid */
  CS_PDN_conf_id_t            pdn_default_conf_id;                        /* default conf_id */

  /* Socket infos */
  atcustom_persistent_SOCKET_context_t   socket[CELLULAR_MAX_SOCKETS];

  /* other infos */
  at_bool_t              modem_at_ready;     /* modem ready to receive AT commands */
  at_bool_t              modem_sim_ready;    /* modem sim ready */
  at_bool_t              sim_pin_code_ready; /* is modem sim ready ? */
  csint_SIMState_t       sim_state;          /* modem sim status */
  atcustom_CMEE_n_t      cmee_level;         /* modem CMEE error level */

  /* Ping infos:
   * keep ping input params in persistent context because ping responses are received after end of current SID
   * (assimilated to URC) and these infos can be useful when receving a ping response
   */
  csint_ping_params_t    ping_infos;     /* input ping request */
  CS_Ping_response_t     ping_resp_urc;  /* ping response parameters (assimililated ton an URC) */
  at_bool_t              urc_avail_ping_rsp;

} atcustom_persistent_context_t;

/* atcustom_SID_context_t is a structure to save SID datas
*   received from Cellular service and related infos
*/
typedef struct
{
  /* list of paramaters used for SID */
  CS_ModemConfig_t            modem_config;           /* SID_CS_MODEM_CONFIG */
  csint_modemInit_t           modem_init;             /* SID_CS_INIT_MODEM */
  CS_DeviceInfo_t             *device_info;           /* SID_CS_GET_DEVICE_INFO */
  CS_OperatorSelector_t       write_operator_infos;   /* SID_CS_REGISTER_NET */
  CS_RegistrationStatus_t     read_operator_infos;    /* SID_CS_REGISTER_NET, SID_CS_GET_NETSTATUS */
  CS_SignalQuality_t          *signal_quality;        /* SID_CS_GET_SIGNAL_QUALITY */
  CS_UrcEvent_t               urcEvent;               /* SID_CS_SUSBCRIBE_NET_EVENT, SID_CS_UNSUSBCRIBE_NET_EVENT */
  CS_PSattach_t               attach_status;          /* SID_CS_GET_ATTACHSTATUS */

  csint_socket_data_buffer_t  socketSendData_struct;  /* SID_CS_SEND_DATA */
  CS_Reset_t                  reset_type;             /* SID_CS_RESET */
  CS_PDN_conf_id_t            pdn_conf_id;            /* SID_CS_ACTIVATE_PDN, SID_CS_DEACTIVATE_PDN */
  csint_dns_request_t         *dns_request_infos;     /* SID_CS_DNS_REQ */
  csint_ping_params_t         ping_infos;             /* SID_CS_PING_IP_ADDRESS */

  csint_error_report_t        error_report;           /* used if an error occurs during an SID */
} atcustom_SID_context_t;

typedef struct
{
  /* list of paramaters used for COMMANDS */
  atcustom_CGSN_snt_t           cgsn_write_cmd_param;  /* AT+CGSN <snt> value */
  atcustom_CGATT_state_t        cgatt_write_cmd_param; /* AT+CGATT <state> value */
  atcustom_CxREG_n_t            cxreg_write_cmd_param; /* AT+CREG,AT+CGREG,AT+CEREG <n> value */
  at_bool_t                     command_echo;          /* ATE */
  at_bool_t                     dce_full_resp_format;  /* ATV */
  at_bool_t                     pdn_state;             /* AT+CGACT: PDP state, true to activate, false to deactivate */
  uint32_t                      modem_cid;             /* AT+CGPADDR <cid> value received in current command */
  uint32_t                      baud_rate;             /* AT+IPR <rate> value */
  at_bool_t                     flow_control_cts_rts;  /* AT+IFC used to set: 0,0 (none) or 2,2 (CTS/RTS) */
  uint8_t                       cfun_value;            /* AT+CFUN <fun> value */
} atcustom_CMD_context_t;

typedef struct
{
  /* SOCKET command data mode : SID or cmd datas */
  csint_socket_infos_t      *socket_info;        /* current socket infos */
  csint_socket_data_buffer_t socketReceivedata;  /* for RX data buffer */
  uint32_t                   socket_current_connId;  /* connection ID for current command (only for received cmd) */
  uint32_t                   socket_rx_expected_buf_size; /* expected size of buffer to receive */
  uint32_t                   socket_rx_count_bytes_received; /* count number of char received actually for input buffer */
  csint_socket_cnx_infos_t   *socket_cnx_infos;   /* SID_CS_SOCKET_CNX_STATUS */

  /* variables used for socket strings analyze */
  atcustom_socket_send_state_t     socket_send_state;
  atcustom_socket_receive_state_t  socket_receive_state;
  atcustom_socket_RxData_state_t   socket_RxData_state; /* indicate RX data state */

} atcustom_SOCKET_context_t;

typedef struct
{
  uint32_t                           modem_LUT_size;
  const struct atcustom_LUT_struct   *p_modem_LUT;

  /* received command syntax analysis: state of automaton which analyzes cmd syntax */
  atcustom_modem_SyntaxAutomatonState_t   state_SyntaxAutomaton;

  /* persistent context infos: data lifetime = permanent */
  atcustom_persistent_context_t       persist;

  /* SID context: data lifetime = SID command */
  atcustom_SID_context_t              SID_ctxt;

  /* COMMAND context: data lifetime = AT command */
  atcustom_CMD_context_t              CMD_ctxt;

  /* SOCKET context */
  atcustom_SOCKET_context_t           socket_ctxt;

} atcustom_modem_context_t;

typedef at_status_t (*CmdBuildFuncTypeDef)(atparser_context_t  *p_atp_ctxt,
                                           atcustom_modem_context_t *p_modem_ctxt);
typedef at_action_rsp_t (*CmdAnalyzeFuncTypeDef)(at_context_t *p_at_ctxt,
                                                 atcustom_modem_context_t *p_modem_ctxt,
                                                 IPC_RxMessage_t *p_msg_in,
                                                 at_element_info_t *element_infos);

struct atcm_pdp_type_LUT_struct;
typedef struct atcm_pdp_type_LUT_struct
{
  CS_PDPtype_t      pdp_type;
  AT_CHAR_t         pdp_type_string[16];
} atcm_pdp_type_LUT_t;

struct atcustom_LUT_struct;
typedef struct atcustom_LUT_struct
{
  uint32_t              cmd_id;
  AT_CHAR_t             cmd_str[ATCMD_MAX_NAME_SIZE];
  uint32_t              cmd_timeout;
  CmdBuildFuncTypeDef   cmd_BuildFunc;
  CmdAnalyzeFuncTypeDef rsp_AnalyzeFunc;
} atcustom_LUT_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
const AT_CHAR_t       *atcm_get_CmdStr(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id);
uint32_t               atcm_get_CmdTimeout(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id);
CmdBuildFuncTypeDef    atcm_get_CmdBuildFunc(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id);
CmdAnalyzeFuncTypeDef  atcm_get_CmdAnalyzeFunc(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id);
const AT_CHAR_t       *atcm_get_PDPtypeStr(CS_PDPtype_t pdp_type);

void atcm_program_AT_CMD(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, at_type_t cmd_type, uint32_t cmd_id, atcustom_FinalCmd_t final);
void atcm_program_AT_CMD_ANSWER_OPTIONAL(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, at_type_t cmd_type, uint32_t cmd_id, atcustom_FinalCmd_t final);
void atcm_program_WAIT_EVENT(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final);
void atcm_program_TEMPO(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final);
void atcm_program_NO_MORE_CMD(atparser_context_t *p_atp_ctxt);
void atcm_program_SKIP_CMD(atparser_context_t *p_atp_ctxt);

void atcm_modem_init(atcustom_modem_context_t *p_modem_ctxt);
void atcm_modem_reset(atcustom_modem_context_t *p_modem_ctxt);
uint8_t atcm_modem_Standard_checkEndOfMsgCallback(atcustom_modem_context_t *p_modem_ctxt, uint8_t rxChar);
at_status_t atcm_modem_build_cmd(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
at_status_t atcm_modem_get_rsp(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcm_modem_get_urc(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcm_modem_get_error(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_bool_t atcm_modem_event_received(atcustom_modem_context_t *p_modem_ctxt, CS_ModemEvent_t mdm_evt);

at_status_t atcm_subscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt);
at_status_t atcm_unsubscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt);
void atcm_validate_ping_request(atcustom_modem_context_t *p_modem_ctxt);

void atcm_reset_persistent_context(atcustom_persistent_context_t *p_persistent_ctxt);
void atcm_reset_SID_context(atcustom_SID_context_t *p_sid_ctxt);
void atcm_reset_CMD_context(atcustom_CMD_context_t *p_cmd_ctxt);
void atcm_reset_SOCKET_context(atcustom_modem_context_t *p_modem_ctxt);

at_status_t atcm_searchCmdInLUT(atcustom_modem_context_t *p_modem_ctxt,
                                atparser_context_t  *p_atp_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos);
at_action_rsp_t atcm_check_text_line_cmd(atcustom_modem_context_t *p_modem_ctxt,
                                         at_context_t *p_at_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos);

void atcm_set_modem_data_mode(atcustom_modem_context_t *p_modem_ctxt, at_bool_t val);
at_bool_t atcm_get_modem_data_mode(atcustom_modem_context_t *p_modem_ctxt);
at_status_t atcm_retrieve_SID_parameters(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt);
uint8_t atcm_get_affected_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id);
CS_PDN_conf_id_t atcm_get_configID_for_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, uint8_t modem_cid);

CS_PDN_conf_id_t atcm_get_cid_current_SID(atcustom_modem_context_t *p_modem_ctxt);
void atcm_put_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt, uint8_t modem_cid, csint_ip_addr_info_t  *ip_addr_info);
void atcm_get_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id, csint_ip_addr_info_t  *ip_addr_info);

/* socket functions */
at_status_t atcm_socket_reserve_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t atcm_socket_release_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
uint32_t atcm_socket_get_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
socket_handle_t atcm_socket_get_socket_handle(atcustom_modem_context_t *p_modem_ctxt, uint32_t modemCID);
at_status_t atcm_socket_set_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t atcm_socket_set_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
socket_handle_t atcm_socket_get_hdle_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt);
socket_handle_t atcm_socket_get_hdlr_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t atcm_socket_remaining_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t atcm_socket_remaining_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t atcm_socket_is_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t atcm_socket_set_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);

/* ==========================  Build 3GPP TS 27.007 commands ========================== */
at_status_t fCmdBuild_NoParams(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGSN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CMEE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CPIN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CFUN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_COPS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGATT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CEREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGEREP(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGDCONT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGDATA(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGPADDR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* ==========================  Build V.25ter commands ========================== */
at_status_t fCmdBuild_ATD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_ATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_ATV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_ATX(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_IPR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_IFC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_ESCAPE_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_AT_AND_D(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* ==========================  Analyze 3GPP TS 27.007 commands ========================== */
at_action_rsp_t fRspAnalyze_None(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_Error(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CmeErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CmsErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGMM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGMR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CIMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CEER(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CPIN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_COPS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CNUM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGATT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CEREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CSQ(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CGPADDR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

/* ==========================  Analyze V.25ter commands ========================== */
at_action_rsp_t fRspAnalyze_GSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_IPR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
