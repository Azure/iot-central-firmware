/**
  ******************************************************************************
  * @file    at_custom_modem_specific.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_specific.c module for BG96
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
#ifndef AT_CUSTOM_MODEM_BG96_H
#define AT_CUSTOM_MODEM_BG96_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_custom_common.h"
#include "at_custom_modem.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
/* device specific parameters */
#define MODEM_MAX_SOCKET_TX_DATA_SIZE ((uint32_t)1460U) /* cf AT+QISEND */
#define MODEM_MAX_SOCKET_RX_DATA_SIZE ((uint32_t)1500U) /* cf AT+QIRD */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  /* modem specific commands */
  _AT_QPOWD = (_AT_LAST_GENERIC + 1U), /* */
  _AT_QCFG,                           /* Extended configuration settings */
  _AT_QINDCFG,                        /* URC indication configuration settings */
  _AT_QIND,                           /* URC indication */
  _AT_QUSIM,                          /* URC indication SIM card typpe (SIM or USIM) */
  _AT_CPSMS,                          /* Power Saving Mode Setting */
  _AT_CEDRXS,                         /* e-I-DRX Setting */
  _AT_QNWINFO,                        /* Query Network Information */
  _AT_QENG,                           /* Switch on or off Engineering Mode */
  /* BG96 specific TCP/IP commands */
  _AT_QICSGP,                         /* Configure parameters of a TCP/IP context */
  _AT_QIACT,                          /* Activate a PDP context */
  _AT_QIDEACT,                        /* Deactivate a PDP context */
  _AT_QIOPEN,                         /* Open a socket service */
  _AT_QICLOSE,                        /* Close a socket service */
  _AT_QISEND,                         /* Send Data - waiting for prompt */
  _AT_QISEND_WRITE_DATA,              /* Send Data - writing data */
  _AT_QIRD,                           /* Retrieve the received TCP/IP Data */
  _AT_QICFG,                          /* Configure optionnal parameters */
  _AT_QISTATE,                        /* Query socket service status */
  _AT_QIURC,                          /* TCP/IP URC */
  _AT_SOCKET_PROMPT,                  /* when sending socket data : prompt = "> " */
  _AT_SEND_OK,                        /* socket send data OK */
  _AT_SEND_FAIL,                      /* socket send data problem */
  _AT_QIDNSCFG,                       /* configure address of DNS server */
  _AT_QIDNSGIP,                       /* get IP address by Domain Name */
  _AT_QPING,                          /* ping a remote server */

  /* modem specific events (URC, BOOT, ...) */
  _AT_WAIT_EVENT,
  _AT_BOOT_EVENT,
  _AT_SIMREADY_EVENT,
  _AT_RDY_EVENT,
  _AT_POWERED_DOWN_EVENT,

} ATCustom_BG96_cmdlist_t;

/* device specific parameters */
typedef enum
{
  _QIURC_unknown,
  _QIURC_closed,
  _QIURC_recv,
  _QIURC_incoming_full,
  _QIURC_incoming,
  _QIURC_pdpdeact,
  _QIURC_dnsgip,
} ATCustom_BG96_QIURC_function_t;

typedef enum
{
  _QCFG_unknown,
  _QCFG_gprsattach,
  _QCFG_nwscanseq,
  _QCFG_nwscanmode,
  _QCFG_iotopmode,
  _QCFG_roamservice,
  _QCFG_band,
  _QCFG_servicedomain,
  _QCFG_sgsn,
  _QCFG_msc,
  _QCFG_PDP_DuplicateChk,
  _QCFG_urc_ri_ring,
  _QCFG_urc_ri_smsincoming,
  _QCFG_urc_ri_other,
  _QCFG_signaltype,
  _QCFG_urc_delay,
} ATCustom_BG96_QCFG_function_t;

typedef enum
{
  _QINDCFG_unknown,
  _QINDCFG_all,
  _QINDCFG_csq,
  _QINDCFG_smsfull,
  _QINDCFG_ring,
  _QINDCFG_smsincoming,
} ATCustom_BG96_QINDCFG_function_t;


typedef uint32_t ATCustom_BG96_QCFGbandGSM_t;
#define _QCFGbandGSM_NoChange ((ATCustom_BG96_QCFGbandGSM_t) 0x0)
#define _QCFGbandGSM_900      ((ATCustom_BG96_QCFGbandGSM_t) 0x1)
#define _QCFGbandGSM_1800     ((ATCustom_BG96_QCFGbandGSM_t) 0x2)
#define _QCFGbandGSM_850      ((ATCustom_BG96_QCFGbandGSM_t) 0x4)
#define _QCFGbandGSM_1900     ((ATCustom_BG96_QCFGbandGSM_t) 0x8)
#define _QCFGbandGSM_any      ((ATCustom_BG96_QCFGbandGSM_t) 0xF)

typedef uint64_t ATCustom_BG96_QCFGbandCatM1_t;
#define _QCFGbandCatM1_B1       ((ATCustom_BG96_QCFGbandCatM1_t) 0x1)
#define _QCFGbandCatM1_B2       ((ATCustom_BG96_QCFGbandCatM1_t) 0x2)
#define _QCFGbandCatM1_B3       ((ATCustom_BG96_QCFGbandCatM1_t) 0x4)
#define _QCFGbandCatM1_B4       ((ATCustom_BG96_QCFGbandCatM1_t) 0x8)
#define _QCFGbandCatM1_B5       ((ATCustom_BG96_QCFGbandCatM1_t) 0x10)
#define _QCFGbandCatM1_B8       ((ATCustom_BG96_QCFGbandCatM1_t) 0x80)
#define _QCFGbandCatM1_B12      ((ATCustom_BG96_QCFGbandCatM1_t) 0x800)
#define _QCFGbandCatM1_B13      ((ATCustom_BG96_QCFGbandCatM1_t) 0x1000)
#define _QCFGbandCatM1_B18      ((ATCustom_BG96_QCFGbandCatM1_t) 0x20000)
#define _QCFGbandCatM1_B19      ((ATCustom_BG96_QCFGbandCatM1_t) 0x40000)
#define _QCFGbandCatM1_B20      ((ATCustom_BG96_QCFGbandCatM1_t) 0x80000)
#define _QCFGbandCatM1_B26      ((ATCustom_BG96_QCFGbandCatM1_t) 0x2000000)
#define _QCFGbandCatM1_B28      ((ATCustom_BG96_QCFGbandCatM1_t) 0x8000000)
#define _QCFGbandCatM1_B39      ((ATCustom_BG96_QCFGbandCatM1_t) 0x4000000000)
#define _QCFGbandCatM1_NoChange ((ATCustom_BG96_QCFGbandCatM1_t) 0x40000000)
#define _QCFGbandCatM1_any      ((ATCustom_BG96_QCFGbandCatM1_t) 0x400a0e189f)

typedef uint64_t ATCustom_BG96_QCFGbandCatNB1_t;
#define _QCFGbandCatNB1_B1       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x1)
#define _QCFGbandCatNB1_B2       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x2)
#define _QCFGbandCatNB1_B3       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x4)
#define _QCFGbandCatNB1_B4       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x8)
#define _QCFGbandCatNB1_B5       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x10)
#define _QCFGbandCatNB1_B8       ((ATCustom_BG96_QCFGbandCatNB1_t) 0x80)
#define _QCFGbandCatNB1_B12      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x800)
#define _QCFGbandCatNB1_B13      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x1000)
#define _QCFGbandCatNB1_B18      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x20000)
#define _QCFGbandCatNB1_B19      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x40000)
#define _QCFGbandCatNB1_B20      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x80000)
#define _QCFGbandCatNB1_B26      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x2000000)
#define _QCFGbandCatNB1_B28      ((ATCustom_BG96_QCFGbandCatNB1_t) 0x8000000)
#define _QCFGbandCatNB1_NoChange ((ATCustom_BG96_QCFGbandCatNB1_t) 0x40000000)
#define _QCFGbandCatNB1_any      ((ATCustom_BG96_QCFGbandCatNB1_t) 0xA0E189F)

typedef uint32_t ATCustom_BG96_QCFGiotopmode_t;
#define _QCFGiotopmodeCatM1       ((ATCustom_BG96_QCFGiotopmode_t) 0x0)
#define _QCFGiotopmodeCatNB1      ((ATCustom_BG96_QCFGiotopmode_t) 0x1)
#define _QCFGiotopmodeCatM1CatNB1 ((ATCustom_BG96_QCFGiotopmode_t) 0x2)

typedef uint32_t ATCustom_BG96_QCFGscanmode_t;
#define _QCFGscanmodeAuto     ((ATCustom_BG96_QCFGscanmode_t) 0x0)
#define _QCFGscanmodeGSMOnly  ((ATCustom_BG96_QCFGscanmode_t) 0x1)
#define _QCFGscanmodeLTEOnly  ((ATCustom_BG96_QCFGscanmode_t) 0x3)

typedef uint32_t ATCustom_BG96_QCFGscanseq_t;
/* individual values */
#define  _QCFGscanseqAuto        ((ATCustom_BG96_QCFGscanseq_t) 0x0)
#define  _QCFGscanseqGSM         ((ATCustom_BG96_QCFGscanseq_t) 0x1)
#define  _QCFGscanseqLTECatM1    ((ATCustom_BG96_QCFGscanseq_t) 0x2)
#define  _QCFGscanseqLTECatNB1   ((ATCustom_BG96_QCFGscanseq_t) 0x3)
/* combined values */
#define  _QCFGscanseq_GSM_M1_NB1 ((ATCustom_BG96_QCFGscanseq_t) 0x010203)
#define  _QCFGscanseq_GSM_NB1_M1 ((ATCustom_BG96_QCFGscanseq_t) 0x010302)
#define  _QCFGscanseq_M1_GSM_NB1 ((ATCustom_BG96_QCFGscanseq_t) 0x020103)
#define  _QCFGscanseq_M1_NB1_GSM ((ATCustom_BG96_QCFGscanseq_t) 0x020301)
#define  _QCFGscanseq_NB1_GSM_M1 ((ATCustom_BG96_QCFGscanseq_t) 0x030102)
#define  _QCFGscanseq_NB1_M1_GSM ((ATCustom_BG96_QCFGscanseq_t) 0x030201)

typedef struct
{
  ATCustom_BG96_QCFGscanseq_t    nw_scanseq;
  ATCustom_BG96_QCFGscanmode_t   nw_scanmode;
  ATCustom_BG96_QCFGiotopmode_t  iot_op_mode;
  ATCustom_BG96_QCFGbandGSM_t    gsm_bands;
  ATCustom_BG96_QCFGbandCatM1_t  CatM1_bands;
  ATCustom_BG96_QCFGbandCatNB1_t CatNB1_bands;
} ATCustom_BG96_mode_band_config_t;


typedef enum
{
  /* Engineering Mode - Cell Type information */
  _QENGcelltype_ServingCell    = 0x0, /* get 2G or 3G serving cell information */
  _QENGcelltype_NeighbourCell  = 0x1, /* get 2G or 3G neighbour cell information */
  _QENGcelltype_PSinfo         = 0x2, /* get 2G or 3G cell information during packet switch connected */
} ATCustom_BG96_QENGcelltype_t;

typedef enum
{
  /* QIOPEN servic type parameter */
  _QIOPENservicetype_TCP_Client  = 0x0, /* get 2G or 3G serving cell information */
  _QIOPENservicetype_UDP_Client  = 0x1, /* get 2G or 3G neighbour cell information */
  _QIOPENservicetype_TCP_Server  = 0x2, /* get 2G or 3G cell information during packet switch connected */
  _QIOPENservicetype_UDP_Server  = 0x3, /* get 2G or 3G cell information during packet switch connected */
} ATCustom_BG96_QIOPENservicetype_t;

typedef struct
{
  at_bool_t finished;
  at_bool_t wait_header; /* indicate we are waiting for +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl> */
  uint32_t  error;
  uint32_t  ip_count;
  uint32_t  ttl;
  AT_CHAR_t hostIPaddr[MAX_SIZE_IPADDR]; /* = host_name parameter from CS_DnsReq_t */
} bg96_qiurc_dnsgip_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
void        ATCustom_BG96_init(atparser_context_t *p_atp_ctxt);
uint8_t     ATCustom_BG96_checkEndOfMsgCallback(uint8_t rxChar);
at_status_t ATCustom_BG96_getCmd(atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t ATCustom_BG96_extractElement(atparser_context_t *p_atp_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos);
at_action_rsp_t ATCustom_BG96_analyzeCmd(at_context_t *p_at_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos);
at_action_rsp_t ATCustom_BG96_analyzeParam(at_context_t *p_at_ctxt,
                                           IPC_RxMessage_t *p_msg_in,
                                           at_element_info_t *element_infos);
at_action_rsp_t ATCustom_BG96_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);

at_status_t ATCustom_BG96_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_BG96_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_BG96_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_BG96_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
