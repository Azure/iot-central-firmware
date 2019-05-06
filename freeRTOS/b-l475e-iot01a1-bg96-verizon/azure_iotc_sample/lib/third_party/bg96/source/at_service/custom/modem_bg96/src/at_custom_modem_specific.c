/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          BG96 Quectel modem: LTE-cat-M1 or LTE-cat.NB1(=NB-IOT) or GSM
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

/* AT commands format
 * AT+<X>=?    : TEST COMMAND
 * AT+<X>?     : READ COMMAND
 * AT+<X>=...  : WRITE COMMAND
 * AT+<X>      : EXECUTION COMMAND
*/

/* BG96 COMPILATION FLAGS to define in project option if needed:*/
#define BG96_OPTION_CHECK_CPIN /* uncomment this line to activate CPIN check */

/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "at_custom_common.h"
#include "at_custom_modem.h"
#include "at_custom_modem_specific.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

#if defined(USE_MODEM_BG96)
#if (defined(HWREF_AKORIOT_BG96) || defined(HWREF_B_CELL_BG96_V2))
#else
#error Hardware reference not specified
#endif
#endif /* USE_MODEM_BG96 */

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P1, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("BG96:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("BG96:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("BG96 API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("BG96 ERROR:" format "\n\r", ## args);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* TWO MACROS USED TO SIMPLIFY CODE WHEN MULTIPLE PARAMETERS EXPECTED FOR AN AT-COMMAND ANSWER */
#define START_PARAM_LOOP()  PrintDBG("rank = %d",element_infos->param_rank)\
                            uint8_t exitcode = 0U;\
                            at_endmsg_t msg_end = ATENDMSG_NO;\
                            do\
                            {\
                              if (msg_end == ATENDMSG_YES) {exitcode = 1U;}

#define END_PARAM_LOOP()    msg_end = atcc_extractElement(p_at_ctxt, p_msg_in, element_infos);\
                            if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
                          } while (exitcode == 0U);

/* Private defines -----------------------------------------------------------*/
/* ###########################  START CUSTOMIZATION PART  ########################### */
/*
List of bands parameters (cf .h file to see the list of enum values for each parameter)
  - BG96_BAND_GSM    : hexadecimal value that specifies the GSM frequency band (cf AT+QCFG="band")
  - BG96_BAND_CAT_M1 : hexadecimal value that specifies the LTE Cat.M1 frequency band (cf AT+QCFG="band")
  - BG96_BAND_CAT_NB1: hexadecimal value that specifies the LTE Cat.NB1 frequency band (cf AT+QCFG="band")
  - BG96_IOTOPMODE   : network category to be searched under LTE network (cf AT+QCFG="iotopmode")
  - BG96_SCANSEQ     : network search sequence (GSM, Cat.M1, Cat.NB1) (cf AT+QCFG="nwscanseq")
  - BG96_SCANMODE    : network to be searched (cf AT+QCFG="nwscanmode")
*/
#if (BG96_SET_BANDS == 0)
/* set values default values to avoid compilation error
 *  flag is defined in plf_modem_config.h
 */
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1CatNB1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_M1_NB1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
#endif /*(BG96_SET_BANDS == 0)*/

#define BG96_PDP_DUPLICATECHK_ENABLE ((uint8_t)0) /* parameter of AT+QCFG="PDP/DuplicateChk": 0 to refuse, 1 to allow */

#define BG96_DEFAULT_TIMEOUT  ((uint32_t)15000)
#define BG96_RDY_TIMEOUT      ((uint32_t)30000)
#define BG96_SIMREADY_TIMEOUT ((uint32_t)5000)
#define BG96_ESCAPE_TIMEOUT   ((uint32_t)1000)  /* maximum time allowed to receive a response to an Escape command */
#define BG96_COPS_TIMEOUT     ((uint32_t)180000) /* 180 sec */
#define BG96_CGATT_TIMEOUT    ((uint32_t)140000) /* 140 sec */
#define BG96_CGACT_TIMEOUT    ((uint32_t)150000) /* 150 sec */
#define BG96_ATH_TIMEOUT      ((uint32_t)90000) /* 90 sec */
#define BG96_AT_TIMEOUT       ((uint32_t)3000)  /* timeout for AT */
#define BG96_SOCKET_PROMPT_TIMEOUT ((uint32_t)10000)
#define BG96_QIOPEN_TIMEOUT   ((uint32_t)150000) /* 150 sec */
#define BG96_QICLOSE_TIMEOUT  ((uint32_t)150000) /* 150 sec */
#define BG96_QIACT_TIMEOUT    ((uint32_t)150000) /* 150 sec */
#define BG96_QIDEACT_TIMEOUT  ((uint32_t)40000) /* 40 sec */
#define BG96_QNWINFO_TIMEOUT  ((uint32_t)1000) /* 1000ms */
#define BG96_QIDNSGIP_TIMEOUT ((uint32_t)60000) /* 60 sec */
#define BG96_QPING_TIMEOUT    ((uint32_t)150000) /* 150 sec */

#if !defined(BG96_OPTION_NETWORK_INFO)
/* set default value */
#define BG96_OPTION_NETWORK_INFO (0)
#endif

#if !defined(BG96_OPTION_ENGINEERING_MODE)
/* set default value */
#define BG96_OPTION_ENGINEERING_MODE (0)
#endif

/* Global variables ----------------------------------------------------------*/
/* BG96 Modem device context */
atcustom_modem_context_t BG96_ctxt;

/* Commands Look-up table for AT+QCFG */
const AT_CHAR_t BG96_QCFG_LUT[][32] =
{
  /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
  {"unknown"}, /* _QCFG_gprsattach */
  {"gprsattach"}, /* _QCFG_gprsattach */
  {"nwscanseq"}, /* _QCFG_nwscanseq */
  {"nwscanmode"}, /* _QCFG_nwscanmode */
  {"iotopmode"}, /* _QCFG_iotopmode */
  {"roamservice"}, /* _QCFG_roamservice */
  {"band"}, /* _QCFG_band */
  {"servicedomain"}, /* _QCFG_servicedomain */
  {"sgsn"}, /* _QCFG_sgsn */
  {"msc"}, /* _QCFG_msc */
  {"pdp/duplicatechk"}, /* _QCFG_PDP_DuplicateChk */
  {"urc/ri/ring"}, /* _QCFG_urc_ri_ring */
  {"urc/ri/smsincoming"}, /* _QCFG_urc_ri_smsincoming */
  {"urc/ri/other"}, /* _QCFG_urc_ri_other */
  {"signaltype"}, /* _QCFG_signaltype */
  {"urc/delay"}, /* _QCFG_urc_delay */
};

/* Private variables ---------------------------------------------------------*/

/* Array to convert AT+QIOPEN service type parameter to a string
*  need to be aligned with ATCustom_BG96_QIOPENservicetype_t
*/
static const AT_CHAR_t *const bg96_array_QIOPEN_service_type[] = { ((uint8_t *)"TCP"), ((uint8_t *)"UDP"), ((uint8_t *)"TCP LISTENER"), ((uint8_t *)"UDP_LISTENER"), }; /* all const */

/* --- VARIABLE SHARED TO SET BG96 SPECIFIC COMMANDS  */
static ATCustom_BG96_QCFG_function_t     BG96_QCFG_command_param = _QCFG_unknown;
static at_bool_t                         BG96_QCFG_command_write = AT_FALSE;
static ATCustom_BG96_QINDCFG_function_t  BG96_QINDCFG_command_param = _QINDCFG_unknown;
static ATCustom_BG96_QENGcelltype_t      BG96_QENG_command_param = _QENGcelltype_ServingCell;

/* --- VARIABLES SHARED IN THIS FILE  */
/* memorize current BG96 mode and bands configuration */
static ATCustom_BG96_mode_band_config_t  bg96_mode_and_bands_config;
/* memorize if waiting for QIOPEN */
static at_bool_t bg96_waiting_qiopen = AT_FALSE;
static uint8_t bg96_current_qiopen_socket_connected = 0U;
/* memorize if QICSGP write command is a config or a query command */
static at_bool_t bg96_QICGSP_config_command = AT_TRUE;
/* memorize infos received in the URC +QIURC:"dnsgip" */
static bg96_qiurc_dnsgip_t bg96_qiurc_dnsgip;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
/* check if request PDN to activate is already active (QIACT) */
static at_bool_t bg96_pdn_already_active;
#endif /* USE_SOCKETS_TYPE */

/* Socket Data receive: to analyze size received in data header */
static AT_CHAR_t SocketHeaderDataRx_Buf[4];
static uint8_t SocketHeaderDataRx_Cpt;
static const uint8_t QIRD_string[] = "+QIRD";
static uint8_t QIRD_Counter = 0U;

/* ###########################  END CUSTOMIZATION PART  ########################### */

/* Private function prototypes -----------------------------------------------*/
/* ###########################  START CUSTOMIZATION PART  ########################### */
static void reinitSyntaxAutomaton_bg96(void);
static void reset_variables_bg96(void);
static void init_bg96_qiurc_dnsgip(void);
static void display_decoded_GSM_bands(uint32_t gsm_bands);
static void display_decoded_CatM1_bands(uint64_t CatM1_bands);
static void display_decoded_CatNB1_bands(uint64_t CatNB1_bands);
static void display_user_friendly_mode_and_bands_config(void);
static uint8_t display_if_active_band(ATCustom_BG96_QCFGscanseq_t scanseq,
                                      uint8_t rank, uint8_t catM1_on, uint8_t catNB1_on, uint8_t gsm_on);
static void socketHeaderRX_reset(void);
static void SocketHeaderRX_addChar(char *rxchar);
static uint16_t SocketHeaderRX_getSize(void);
static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt);

/* BG96 build commands overriding common function  or specific */
static at_status_t fCmdBuild_ATD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_CGSN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QPOWD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QINDCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_CPSMS_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_CEDRXS_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QENG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QICSGP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QIACT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QICFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QIOPEN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QICLOSE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QISEND_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QISEND_WRITE_DATA_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QIRD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QISTATE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_CGDCONT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QIDNSCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QIDNSGIP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
static at_status_t fCmdBuild_QPING_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* BG96 specific analyze commands */
static at_action_rsp_t fRspAnalyze_CPIN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_CFUN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIND_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIACT_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QINDCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIOPEN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                               IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIURC_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIRD_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QIRD_data_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QISTATE_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_QPING_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
static at_action_rsp_t fRspAnalyze_Error_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

/* Commands Look-up table */
const atcustom_LUT_t ATCMD_BG96_LUT[] =
{
  /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
  {_AT,             "",             BG96_AT_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_OK,          "OK",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_CONNECT,     "CONNECT",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_RING,        "RING",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_NO_CARRIER,  "NO CARRIER",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_ERROR,       "ERROR",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_BG96},
  {_AT_NO_DIALTONE, "NO DIALTONE",  BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_BUSY,        "BUSY",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_NO_ANSWER,   "NO ANSWER",    BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_CME_ERROR,   "+CME ERROR",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_BG96},
  {_AT_CMS_ERROR,   "+CMS ERROR",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CmsErr},

  /* GENERIC MODEM commands */
  {_AT_CGMI,        "+CGMI",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMI},
  {_AT_CGMM,        "+CGMM",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMM},
  {_AT_CGMR,        "+CGMR",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMR},
  {_AT_CGSN,        "+CGSN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGSN_BG96,  fRspAnalyze_CGSN},
  {_AT_GSN,         "+GSN",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_GSN},
  {_AT_CIMI,        "+CIMI",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CIMI},
  {_AT_CEER,        "+CEER",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEER},
  {_AT_CMEE,        "+CMEE",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CMEE,       fRspAnalyze_None},
  {_AT_CPIN,        "+CPIN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CPIN,       fRspAnalyze_CPIN_BG96},
  {_AT_CFUN,        "+CFUN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CFUN,       fRspAnalyze_CFUN_BG96},
  {_AT_COPS,        "+COPS",        BG96_COPS_TIMEOUT,     fCmdBuild_COPS,       fRspAnalyze_COPS},
  {_AT_CNUM,        "+CNUM",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CNUM},
  {_AT_CGATT,       "+CGATT",       BG96_CGATT_TIMEOUT,    fCmdBuild_CGATT,      fRspAnalyze_CGATT},
  {_AT_CGPADDR,     "+CGPADDR",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGPADDR,    fRspAnalyze_CGPADDR},
  {_AT_CEREG,       "+CEREG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_CEREG,      fRspAnalyze_CEREG},
  {_AT_CREG,        "+CREG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CREG,       fRspAnalyze_CREG},
  {_AT_CGREG,       "+CGREG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGREG,      fRspAnalyze_CGREG},
  {_AT_CSQ,         "+CSQ",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CSQ},
  {_AT_CGDCONT,     "+CGDCONT",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT_BG96,    fRspAnalyze_None},
  {_AT_CGACT,       "+CGACT",       BG96_CGACT_TIMEOUT,    fCmdBuild_CGACT,      fRspAnalyze_None},
  {_AT_CGDATA,      "+CGDATA",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDATA,     fRspAnalyze_None},
  {_AT_CGEREP,      "+CGEREP",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGEREP,     fRspAnalyze_None},
  {_AT_CGEV,        "+CGEV",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGEV},
  {_ATD,            "D",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATD_BG96,   fRspAnalyze_None},
  {_ATE,            "E",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATE,        fRspAnalyze_None},
  {_ATH,            "H",            BG96_ATH_TIMEOUT,      fCmdBuild_NoParams,   fRspAnalyze_None},
  {_ATO,            "O",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_ATV,            "V",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATV,        fRspAnalyze_None},
  {_ATX,            "X",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATX,        fRspAnalyze_None},
  {_AT_ESC_CMD,     "+++",          BG96_ESCAPE_TIMEOUT,   fCmdBuild_ESCAPE_CMD, fRspAnalyze_None},
  {_AT_IPR,         "+IPR",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_IPR,        fRspAnalyze_IPR},
  {_AT_IFC,         "+IFC",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_IFC,        fRspAnalyze_None},
  {_AT_AND_W,       "&W",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_AND_D,       "&D",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_AT_AND_D,   fRspAnalyze_None},

  /* MODEM SPECIFIC COMMANDS */
  {_AT_QPOWD,       "+QPOWD",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_QPOWD_BG96, fRspAnalyze_None},
  {_AT_QCFG,        "+QCFG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QCFG_BG96,  fRspAnalyze_QCFG_BG96},
  {_AT_QINDCFG,    "+QINDCFG",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_QINDCFG_BG96,  fRspAnalyze_QINDCFG_BG96},
  {_AT_QIND,        "+QIND",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_QIND_BG96},
  {_AT_QUSIM,       "+QUSIM",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_CPSMS,       "+CPSMS",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_CPSMS_BG96, fRspAnalyze_None},
  {_AT_CEDRXS,      "+CEDRXS",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_CEDRXS_BG96, fRspAnalyze_None},
  {_AT_QNWINFO,     "+QNWINFO",     BG96_QNWINFO_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_None},
  {_AT_QENG,        "+QENG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QENG_BG96, fRspAnalyze_None},

  {_AT_QICSGP,      "+QICSGP",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QICSGP_BG96, fRspAnalyze_None},
  {_AT_QIACT,       "+QIACT",       BG96_QIACT_TIMEOUT,    fCmdBuild_QIACT_BG96,  fRspAnalyze_QIACT_BG96},
  {_AT_QIOPEN,      "+QIOPEN",      BG96_QIOPEN_TIMEOUT,   fCmdBuild_QIOPEN_BG96, fRspAnalyze_QIOPEN_BG96},
  {_AT_QICLOSE,     "+QICLOSE",     BG96_QICLOSE_TIMEOUT,  fCmdBuild_QICLOSE_BG96, fRspAnalyze_None},
  {_AT_QISEND,      "+QISEND",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_BG96, fRspAnalyze_None},
  {_AT_QISEND_WRITE_DATA,  "",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_WRITE_DATA_BG96, fRspAnalyze_None},
  {_AT_QIRD,        "+QIRD",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QIRD_BG96, fRspAnalyze_QIRD_BG96},
  {_AT_QICFG,       "+QICFG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_QICFG_BG96, fRspAnalyze_None},
  {_AT_QISTATE,     "+QISTATE",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISTATE_BG96, fRspAnalyze_QISTATE_BG96},
  {_AT_QIURC,       "+QIURC",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QIURC_BG96},
  {_AT_SOCKET_PROMPT, "> ",         BG96_SOCKET_PROMPT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_SEND_OK,      "SEND OK",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_SEND_FAIL,    "SEND FAIL",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_QIDNSCFG,     "+QIDNSCFG",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_QIDNSCFG_BG96, fRspAnalyze_None},
  {_AT_QIDNSGIP,     "+QIDNSGIP",   BG96_QIDNSGIP_TIMEOUT, fCmdBuild_QIDNSGIP_BG96, fRspAnalyze_None},
  {_AT_QPING,        "+QPING",      BG96_QPING_TIMEOUT,    fCmdBuild_QPING_BG96, fRspAnalyze_QPING_BG96},

  /* MODEM SPECIFIC EVENTS */
  {_AT_WAIT_EVENT,     "",          BG96_DEFAULT_TIMEOUT,        fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_BOOT_EVENT,     "",          BG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_RDY_EVENT,      "RDY",       BG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
  {_AT_POWERED_DOWN_EVENT, "POWERED DOWN",       BG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
};
#define size_ATCMD_BG96_LUT ((uint16_t) (sizeof (ATCMD_BG96_LUT) / sizeof (atcustom_LUT_t)))

/* ###########################  END CUSTOMIZATION PART  ########################### */


/* Functions Definition ------------------------------------------------------*/
void ATCustom_BG96_init(atparser_context_t *p_atp_ctxt)
{
  /* common init */
  atcm_modem_init(&BG96_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  BG96_ctxt.modem_LUT_size = size_ATCMD_BG96_LUT;
  BG96_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_BG96_LUT;

  /* override default termination string for AT command: <CR> */
  sprintf((char *)p_atp_ctxt->endstr, "\r");

  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

uint8_t ATCustom_BG96_checkEndOfMsgCallback(uint8_t rxChar)
{
  uint8_t last_char = 0U;

  switch (BG96_ctxt.state_SyntaxAutomaton)
  {
    case WAITING_FOR_INIT_CR:
    {
      /* waiting for first valid <CR>, char received before are considered as trash */
      if ('\r' == rxChar)
      {
        /* current     : xxxxx<CR>
        *  command format : <CR><LF>xxxxxxxx<CR><LF>
        *  waiting for : <LF>
        */
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      break;
    }

    case WAITING_FOR_CR:
    {
      if ('\r' == rxChar)
      {
        /* current     : xxxxx<CR>
        *  command format : <CR><LF>xxxxxxxx<CR><LF>
        *  waiting for : <LF>
        */
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      break;
    }

    case WAITING_FOR_LF:
    {
      /* waiting for <LF> */
      if ('\n' == rxChar)
      {
        /*  current        : <CR><LF>
        *   command format : <CR><LF>xxxxxxxx<CR><LF>
        *   waiting for    : x or <CR>
        */
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_FIRST_CHAR;
        last_char = 1U;
        QIRD_Counter = 0U;
      }
      break;
    }

    case WAITING_FOR_FIRST_CHAR:
    {
      if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_waiting_header)
      {
        /* Socket Data RX - waiting for Header: we are waiting for +QIRD
        *
        * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *    ^- waiting this string
        */
        if (rxChar == QIRD_string[QIRD_Counter])
        {
          QIRD_Counter++;
          if (QIRD_Counter == (uint8_t) strlen((const char *)QIRD_string))
          {
            /* +QIRD detected, next step */
            socketHeaderRX_reset();
            BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_header;
          }
        }
      }

      /* NOTE:
       * if we are in socket_RxData_state = SocketRxDataState_waiting_header, we are waiting for +QIRD (test above)
       * but if we receive another message, we need to evacuate it without modifying socket_RxData_state
       * That's why we are nt using "else if" here, if <CR> if received before +QIND, it means that we have received
       * something else
       */
      if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_header)
      {
        /* Socket Data RX - Header received: we are waiting for second <CR>
        *
        * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *         ^- retrieving this size
        *             ^- waiting this <CR>
        */
        if ('\r' == rxChar)
        {
          /* second <CR> detected, we have received data header
          *  now waiting for <LF>, then start to receive socket datas
          *  Verify that size received in header is the expected one
          */
          uint16_t size_from_header = SocketHeaderRX_getSize();
          if (BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size != size_from_header)
          {
            /* update buffer size received - should not happen */
            BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size = size_from_header;
          }
          BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
          BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_data;
        }
        else if ((rxChar >= '0') && (rxChar <= '9'))
        {
          /* receiving size of data in header */
          SocketHeaderRX_addChar((char *)&rxChar);
        }
        else {/* nothing to do */ }
      }
      else if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_data)
      {
        /* receiving socket data: do not analyze char, just count expected size
        *
        * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *.                  ^- start to read data: count char
        */
        BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_SOCKET_DATA;
        /* check if full buffer has been received */
        if (BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
        {
          BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
          BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
        }
      }
      /* waiting for <CR> or x */
      else if ('\r' == rxChar)
      {
        /*   current        : <CR>
        *   command format : <CR><LF>xxxxxxxx<CR><LF>
        *   waiting for    : <LF>
        */
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      else {/* nothing to do */ }
      break;
    }

    case WAITING_FOR_SOCKET_DATA:
      BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
      /* check if full buffer has been received */
      if (BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
      {
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
      }
      break;

    default:
      /* should not happen */
      break;
  }

  /* ###########################  START CUSTOMIZATION PART  ######################### */
  /* if modem does not use standard syntax or has some specificities, replace previous
  *  function by a custom function
  */
  if (last_char == 0U)
  {
    /* BG96 special cases
    *
    *  SOCKET MODE: when sending DATA using AT+QISEND, we are waiting for socket prompt "<CR><LF>> "
    *               before to send DATA. Then we should receive "OK<CR><LF>".
    */

    if (BG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
    {
      switch (BG96_ctxt.socket_ctxt.socket_send_state)
      {
        case SocketSendState_WaitingPrompt1st_greaterthan:
        {
          /* detecting socket prompt first char: "greater than" */
          if ('>' == rxChar)
          {
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt2nd_space;
          }
          break;
        }

        case SocketSendState_WaitingPrompt2nd_space:
        {
          /* detecting socket prompt second char: "space" */
          if (' ' == rxChar)
          {
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_Prompt_Received;
            last_char = 1U;
          }
          else
          {
            /* if char iommediatly after "greater than" is not a "space", reinit state */
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
          }
          break;
        }

        default:
          break;
      }
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (last_char);
}

at_status_t ATCustom_BG96_getCmd(atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  at_status_t retval = ATSTATUS_OK;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  PrintAPI("enter ATCustom_BG96_getCmd() for SID %d", curSID)

  /* retrieve parameters from SID command (will update SID_ctxt) */
  if (atcm_retrieve_SID_parameters(&BG96_ctxt, p_atp_ctxt) != ATSTATUS_OK)
  {
    return (ATSTATUS_ERROR);
  }

  /* new command: reset command context */
  atcm_reset_CMD_context(&BG96_ctxt.CMD_ctxt);

  /* For each SID, athe sequence of AT commands to send id defined (it can be dynamic)
  * Determine and prepare the next command to send for this SID
  */

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  if (curSID == SID_CS_CHECK_CNX)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_MODEM_CONFIG)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if ((curSID == SID_CS_POWER_ON) || (curSID == SID_CS_RESET))
  {
    uint8_t common_start_sequence_step = 1U;

    /* POWER_ON and RESET are almost the same, specific differences are managed case by case */

    /* for reset, only HW reset is supported */
    if ((curSID == SID_CS_RESET) && (BG96_ctxt.SID_ctxt.reset_type != CS_RESET_HW))
    {
      PrintErr("Reset type (%d) not supported", BG96_ctxt.SID_ctxt.reset_type)
      retval = ATSTATUS_ERROR;
    }
    else
    {
      if (p_atp_ctxt->step == 0U)
      {
        /* reset modem specific variables */
        reset_variables_bg96();

        /* check if RDY has been received */
        if (BG96_ctxt.persist.modem_at_ready == AT_TRUE)
        {
          PrintINFO("Modem START indication already received, continue init sequence...")
          /* now reset modem_at_ready (in case of reception of reset indication) */
          BG96_ctxt.persist.modem_at_ready = AT_FALSE;
          /* if yes, go to next step: jump to POWER ON sequence step */
          p_atp_ctxt->step = common_start_sequence_step;

          if (curSID == SID_CS_RESET)
          {
            /* reinit context for reset case */
            atcm_modem_reset(&BG96_ctxt);
          }
        }
        else
        {
          if (curSID == SID_CS_RESET)
          {
            /* reinit context for reset case */
            atcm_modem_reset(&BG96_ctxt);
          }

          PrintINFO("Modem START indication not received yet, waiting for it...")
          /* wait for RDY (if not received, this is not an error, see below with autobaud) */
          atcm_program_TEMPO(p_atp_ctxt, BG96_RDY_TIMEOUT, INTERMEDIATE_CMD);
        }
      }

      /* start common power ON sequence here */
      if (p_atp_ctxt->step == common_start_sequence_step)
      {
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 1U)
      {
        /* disable echo */
        BG96_ctxt.CMD_ctxt.command_echo = AT_FALSE;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _ATE, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 2U)
      {
        /* request detailled error report */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CMEE, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 3U)
      {
        /* enable full response format */
        BG96_ctxt.CMD_ctxt.dce_full_resp_format = AT_TRUE;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _ATV, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 4U)
      {
        /* deactivate DTR */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_AND_D, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 5U)
      {
        /* set flow control */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_IFC, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 6U)
      {
        /* Read baud rate settings */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_IPR, INTERMEDIATE_CMD);
      }
      /* ----- start specific power ON sequence here ----
      * BG96_AT_Commands_Manual_V2.0
      */
      else if (p_atp_ctxt->step == common_start_sequence_step + 7U)
      {
#if (BG96_SEND_READ_CPSMS == 1)
        /* check power saving mode settings */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CPSMS, INTERMEDIATE_CMD);
#else
        /* do not check power saving mode settings */
        atcm_program_SKIP_CMD(p_atp_ctxt);
#endif
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 8U)
      {
#if (BG96_SEND_READ_CEDRXS == 1)
        /* check e-I-DRX settings */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CEDRXS, INTERMEDIATE_CMD);
#else
        /* do not check e-I-DRX settings */
        atcm_program_SKIP_CMD(p_atp_ctxt);
#endif
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 9U)
      {
#if (BG96_SEND_WRITE_CPSMS == 1)
        /* set power settings mode params */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CPSMS, INTERMEDIATE_CMD);
#else
        /* do not set power settings mode params */
        atcm_program_SKIP_CMD(p_atp_ctxt);
#endif
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 10U)
      {
#if (BG96_SEND_WRITE_CEDRXS == 1)
        /* set e-I-DRX params */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CEDRXS, INTERMEDIATE_CMD);
#else
        /* do not set e-I-DRX params */
        atcm_program_SKIP_CMD(p_atp_ctxt);
#endif
      }
#if (BG96_SET_BANDS == 0)
      /* Only check bands paramaters */
      else if (p_atp_ctxt->step == common_start_sequence_step + 11U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_band;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 12U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_iotopmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 13U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_nwscanseq;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 14U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_nwscanmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, FINAL_CMD);
      }
      else if (p_atp_ctxt->step > common_start_sequence_step + 14U)
      {
        /* error, invalid step */
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* ignore */
      }
#else
      /* Check and set bands paramaters */
      else if (p_atp_ctxt->step == common_start_sequence_step + 11U)
      {
        BG96_QCFG_command_write = AT_TRUE;
        BG96_QCFG_command_param = _QCFG_iotopmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 12U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_iotopmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 13U)
      {
        BG96_QCFG_command_write = AT_TRUE;
        BG96_QCFG_command_param = _QCFG_nwscanseq;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 14U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_nwscanseq;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 15U)
      {
        BG96_QCFG_command_write = AT_TRUE;
        BG96_QCFG_command_param = _QCFG_nwscanmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 16U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param =  _QCFG_nwscanmode;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 17U)
      {
        BG96_QCFG_command_write = AT_TRUE;
        BG96_QCFG_command_param = _QCFG_band;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == common_start_sequence_step + 18U)
      {
        BG96_QCFG_command_write = AT_FALSE;
        BG96_QCFG_command_param = _QCFG_band;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QCFG, FINAL_CMD);
      }
      else if (p_atp_ctxt->step > common_start_sequence_step + 18U)
      {
        /* error, invalid step */
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* ignore */
      }

#endif
    }
  }
  else if (curSID == SID_CS_POWER_OFF)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QPOWD, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_INIT_MODEM)
  {
#if defined(BG96_OPTION_CHECK_CPIN)
    if (p_atp_ctxt->step == 0U)
    {
      /* check is CPIN is requested */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CPIN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      if (BG96_ctxt.persist.sim_pin_code_ready == AT_FALSE)
      {
        if (strlen((const char *)&BG96_ctxt.SID_ctxt.modem_init.pincode.pincode) != 0U)
        {
          /* send PIN value */
          PrintINFO("CPIN required, we send user value to modem")
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CPIN, INTERMEDIATE_CMD);
        }
        else
        {
          /* no PIN provided by user */
          PrintINFO("CPIN required but not provided by user")
          retval = ATSTATUS_ERROR;
        }
      }
      else
      {
        PrintINFO("CPIN not required")
        /* no PIN required, skip cmd and go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CFUN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 3U)
    {
      /* check PDP context parameters */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CGDCONT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CFUN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check PDP context parameters */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CGDCONT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* BG96_OPTION_CHECK_CPIN */
  }
  else if (curSID == SID_CS_GET_DEVICE_INFO)
  {
    if (p_atp_ctxt->step == 0U)
    {
      switch (BG96_ctxt.SID_ctxt.device_info->field_requested)
      {
        case CS_DIF_MANUF_NAME_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CGMI, FINAL_CMD);
          break;

        case CS_DIF_MODEL_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CGMM, FINAL_CMD);
          break;

        case CS_DIF_REV_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CGMR, FINAL_CMD);
          break;

        case CS_DIF_SN_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMEI_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_GSN, FINAL_CMD);
          break;

        case CS_DIF_IMSI_PRESENT:
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CIMI, FINAL_CMD);
          break;

        case CS_DIF_PHONE_NBR_PRESENT:
          /* not AT+CNUM not supported by BG96 */
          retval = ATSTATUS_ERROR;
          break;

        default:
          /* error, invalid step */
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_GET_SIGNAL_QUALITY)
  {
    uint8_t sid_steps = 0U;
#if (BG96_OPTION_NETWORK_INFO == 1)
    sid_steps++;
#endif
#if (BG96_OPTION_ENGINEERING_MODE == 1)
    sid_steps++;
#endif

    atcustom_FinalCmd_t last_cmd_info;
    last_cmd_info = (p_atp_ctxt->step == sid_steps) ? FINAL_CMD : INTERMEDIATE_CMD;

    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_CSQ, last_cmd_info);
    }
#if (BG96_OPTION_NETWORK_INFO == 1)
    else if (p_atp_ctxt->step == 1U)
    {
      /* NB: cmd answer is optionnal ie no error will be raised if no answer received from modem
      *  indeed, if requested here, it's just a bonus and should not generate an error
      */
      atcm_program_AT_CMD_ANSWER_OPTIONAL(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _AT_QNWINFO, last_cmd_info);
    }
#endif
#if (BG96_OPTION_ENGINEERING_MODE == 1)
    else if ((p_atp_ctxt->step == 2U) ||
             ((p_atp_ctxt->step == 1U) && (sid_steps == 1U)))
    {
      BG96_QENG_command_param = _QENGcelltype_ServingCell;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QENG, last_cmd_info);
    }
#endif
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_GET_ATTACHSTATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_REGISTER_NET)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_COPS, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
#if 0
      /* check if actual registration status is the expected one */
      CS_OperatorSelector_t *operatorSelect = &(BG96_ctxt.SID_ctxt.write_operator_infos);
      if (BG96_ctxt.SID_ctxt.read_operator_infos.mode != operatorSelect->mode)
      {
        /* write registration status */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_COPS, INTERMEDIATE_CMD);
      }
      else
      {
        /* skip this step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
#else
      /* due to problem observed on simu: does not register after reboot */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_COPS, INTERMEDIATE_CMD);
#endif
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CEREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 3U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 4U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CGREG, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_GET_NETSTATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CEREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_CGREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 3U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_COPS, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_SUSBCRIBE_NET_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      CS_UrcEvent_t urcEvent = BG96_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG, CGREG or CEREG ? */
      if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        atcm_subscribe_net_event(&BG96_ctxt, p_atp_ctxt);
      }
      else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
      {
        /* if signal quality URC not yet suscbribe */
        if (BG96_ctxt.persist.urc_subscript_signalQuality == CELLULAR_FALSE)
        {
          /* set event as suscribed */
          BG96_ctxt.persist.urc_subscript_signalQuality = CELLULAR_TRUE;

          /* request the URC we want */
          BG96_QINDCFG_command_param = _QINDCFG_csq;
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QINDCFG, FINAL_CMD);
        }
        else
        {
          atcm_program_NO_MORE_CMD(p_atp_ctxt);
        }
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }

    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_UNSUSBCRIBE_NET_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      CS_UrcEvent_t urcEvent = BG96_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG, CGREG or CEREG ? */
      if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        atcm_unsubscribe_net_event(&BG96_ctxt, p_atp_ctxt);
      }
      else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
      {
        /* if signal quality URC suscbribed */
        if (BG96_ctxt.persist.urc_subscript_signalQuality == CELLULAR_TRUE)
        {
          /* set event as unsuscribed */
          BG96_ctxt.persist.urc_subscript_signalQuality = CELLULAR_FALSE;

          /* request the URC we don't want */
          BG96_QINDCFG_command_param = _QINDCFG_csq;
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QINDCFG, FINAL_CMD);
        }
        else
        {
          atcm_program_NO_MORE_CMD(p_atp_ctxt);
        }
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_REGISTER_PDN_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      if (BG96_ctxt.persist.urc_subscript_pdn_event == CELLULAR_FALSE)
      {
        /* set event as suscribed */
        BG96_ctxt.persist.urc_subscript_pdn_event = CELLULAR_TRUE;

        /* request PDN events */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGEREP, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_DEREGISTER_PDN_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      if (BG96_ctxt.persist.urc_subscript_pdn_event == CELLULAR_TRUE)
      {
        /* set event as unsuscribed */
        BG96_ctxt.persist.urc_subscript_pdn_event = CELLULAR_FALSE;

        /* request PDN events */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGEREP, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_ATTACH_PS_DOMAIN)
  {
    if (p_atp_ctxt->step == 0U)
    {
      BG96_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_ATTACHED;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_DETACH_PS_DOMAIN)
  {
    if (p_atp_ctxt->step == 0U)
    {
      BG96_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_DETACHED;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_ACTIVATE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if (p_atp_ctxt->step == 0U)
    {
      /* ckeck PDN state */
      bg96_pdn_already_active = AT_FALSE;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_QIACT, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* PDN activation */
      if (bg96_pdn_already_active == AT_TRUE)
      {
        /* PDN already active - exit */
        PrintINFO("Skip PDN activation (already active)")
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* request PDN activation */
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIACT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* ckeck PDN state */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_QIACT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    /* DATA MODE*/
    if (p_atp_ctxt->step == 0U)
    {
      /* get IP address */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGPADDR, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _ATD, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == SID_CS_DEACTIVATE_PDN)
  {
    /* not implemented yet */
    retval = ATSTATUS_ERROR;
  }
  else if (curSID == SID_CS_DEFINE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if (p_atp_ctxt->step == 0U)
    {
      bg96_QICGSP_config_command = AT_TRUE;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QICSGP, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      bg96_QICGSP_config_command = AT_FALSE;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QICSGP, FINAL_CMD);
    }

    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    /* DATA MODE*/
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGDCONT, FINAL_CMD);
      /* add AT+CGAUTH for username and password if required */
      /* could also use AT+QICSGP */
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == SID_CS_SET_DEFAULT_PDN)
  {
    /* nothing to do here
     * Indeed, default PDN has been saved automatically during analysis of SID command
     * cf function: atcm_retrieve_SID_parameters()
     */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == SID_CS_GET_IP_ADDRESS)
  {
    /* get IP address */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, _AT_QIACT, FINAL_CMD);
#else
    /* DATA MODE*/
    atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGPADDR, FINAL_CMD);
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == SID_CS_DIAL_COMMAND)
  {
    /* SOCKET CONNECTION FOR COMMAND DATA MODE */
    if (p_atp_ctxt->step == 0U)
    {
      /* reserve a modem CID for this socket_handle */
      bg96_current_qiopen_socket_connected = 0U;
      socket_handle_t sockHandle = BG96_ctxt.socket_ctxt.socket_info->socket_handle;
      atcm_socket_reserve_modem_cid(&BG96_ctxt, sockHandle);
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIOPEN, INTERMEDIATE_CMD);
      PrintINFO("For Client Socket Handle=%d : MODEM CID affected=%d",
                sockHandle,
                BG96_ctxt.persist.socket[sockHandle].socket_connId_value)

    }
    else if (p_atp_ctxt->step == 1U)
    {
      if (bg96_current_qiopen_socket_connected == 0U)
      {
        /* Waiting for +QIOPEN urc indicating that socket is open */
        atcm_program_TEMPO(p_atp_ctxt, BG96_QIOPEN_TIMEOUT, INTERMEDIATE_CMD);
      }
      else
      {
        /* socket opened */
        bg96_waiting_qiopen = AT_FALSE;
        /* socket is connected */
        atcm_socket_set_connected(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else  if (p_atp_ctxt->step == 2U)
    {
      if (bg96_current_qiopen_socket_connected == 0U)
      {
        /* QIOPEN NOT RECEIVED,
        *  cf BG96 TCP/IP AT Commands Manual V1.0, paragraph 2.1.4 - 3/
        *  "if the URC cannot be received within 150 seconds, AT+QICLOSE should be used to close
        *   the socket".
        *
        *  then we will have to return an error to cellular service !!! (see next step)
        */
        bg96_waiting_qiopen = AT_FALSE;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QICLOSE, INTERMEDIATE_CMD);
      }
      else
      {
        /* socket opened */
        bg96_waiting_qiopen = AT_FALSE;
        /* socket is connected */
        atcm_socket_set_connected(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else  if (p_atp_ctxt->step == 3U)
    {
      /* if we fall here, it means we have send _AT_QICLOSE on previous step
      *  now inform cellular service that opening has failed => return an error
      */
      /* release the modem CID for this socket_handle */
      atcm_socket_release_modem_cid(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_SEND_DATA)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* Check data size to send */
      if (BG96_ctxt.SID_ctxt.socketSendData_struct.buffer_size > MODEM_MAX_SOCKET_TX_DATA_SIZE)
      {
        PrintErr("Data size to send %ld exceed maximum size %ld",
                 BG96_ctxt.SID_ctxt.socketSendData_struct.buffer_size,
                 MODEM_MAX_SOCKET_TX_DATA_SIZE)
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
        retval = ATSTATUS_ERROR;
      }
      else
      {
        BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QISEND, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* waiting for socket prompt: "<CR><LF>> " */
      if (BG96_ctxt.socket_ctxt.socket_send_state == SocketSendState_Prompt_Received)
      {
        PrintDBG("SOCKET PROMPT ALREADY RECEIVED")
        /* go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        PrintDBG("WAITING FOR SOCKET PROMPT")
        atcm_program_WAIT_EVENT(p_atp_ctxt, BG96_SOCKET_PROMPT_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* socket prompt received, send DATA */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, _AT_QISEND_WRITE_DATA, FINAL_CMD);

      /* reinit automaton to receive answer */
      reinitSyntaxAutomaton_bg96();
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_RECEIVE_DATA)
  {
    if (p_atp_ctxt->step == 0U)
    {
      BG96_ctxt.socket_ctxt.socket_receive_state = SocketRcvState_RequestSize;
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIRD, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check that data size to receive is not null */
      if (BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size != 0U)
      {
        /* check that data size to receive does not exceed maximum size
        *  if it's the case, only request maximum size we can receive
        */
        if (BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size >
            BG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size)
        {
          PrintINFO("Data size available (%ld) exceed buffer maximum size (%ld)",
                    BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size,
                    BG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size)
          BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size = BG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size;
        }

        /* receive datas */
        BG96_ctxt.socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Header;
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIRD, FINAL_CMD);
      }
      else
      {
        /* no datas to receive */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_SOCKET_CLOSE)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* is socket connected ?
      * due to BG96 socket connection mechanism (waiting URC QIOPEN), we can fall here but socket
      * has been already closed if error occurs during connection
      */
      if (atcm_socket_is_connected(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle) == AT_TRUE)
      {
        atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QICLOSE, INTERMEDIATE_CMD);
      }
      else
      {
        /* release the modem CID for this socket_handle */
        atcm_socket_release_modem_cid(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* release the modem CID for this socket_handle */
      atcm_socket_release_modem_cid(&BG96_ctxt, BG96_ctxt.socket_ctxt.socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_DATA_SUSPEND)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* wait for 1 second */
      atcm_program_TEMPO(p_atp_ctxt, 1000U, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* send escape sequence +++ (RAW command type)
      *  CONNECT expected before 1000 ms
      */
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, _AT_ESC_CMD, FINAL_CMD);
      reinitSyntaxAutomaton_bg96();
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_SOCKET_CNX_STATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QISTATE, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_DATA_RESUME)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, _ATO, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_DNS_REQ)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIDNSCFG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* intialize +QIURC "dnsgip" parameters */
      init_bg96_qiurc_dnsgip();
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QIDNSGIP, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* do we have received a valid DNS response ? */
      if ((bg96_qiurc_dnsgip.finished == AT_TRUE) && (bg96_qiurc_dnsgip.error == 0U))
      {
        /* yes */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* not yet, waiting for DNS informations */
        atcm_program_TEMPO(p_atp_ctxt, BG96_QIDNSGIP_TIMEOUT, FINAL_CMD);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == SID_CS_SUSBCRIBE_MODEM_EVENT)
  {
    /* nothing to do here
     * Indeed, default modem events subscribed havebeen saved automatically during analysis of SID command
     * cf function: atcm_retrieve_SID_parameters()
     */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == SID_CS_PING_IP_ADDRESS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_QPING, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */
  else
  {
    PrintErr("Error, invalid command ID %d", curSID)
    retval = ATSTATUS_ERROR;
  }

  /* if no error, build the command to send */
  if (retval == ATSTATUS_OK)
  {
    retval = atcm_modem_build_cmd(&BG96_ctxt, p_atp_ctxt, p_ATcmdTimeout);
  }

  return (retval);
}

at_endmsg_t ATCustom_BG96_extractElement(atparser_context_t *p_atp_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  uint8_t exit_loop = 0U;
  uint16_t idx, start_idx;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PrintAPI("enter ATCustom_BG96_extractElement()")
  PrintDBG("input message: size=%d ", p_msg_in->size)

  /* if this is beginning of message, check that message header is correct and jump over it */
  if (*p_parseIndex == 0U)
  {
    /* ###########################  START CUSTOMIZATION PART  ########################### */
    /* MODEM RESPONSE SYNTAX:
    * <CR><LF><response><CR><LF>
    *
    */
    start_idx = 0U;
    /* search initial <CR><LF> sequence (for robustness) */
    if ((p_msg_in->buffer[0] == '\r') && (p_msg_in->buffer[1] == '\n'))
    {
      /* <CR><LF> sequence has been found, it is a command line */
      PrintDBG("cmd init sequence <CR><LF> found - break")
      *p_parseIndex = 2U;
      start_idx = 2U;
    }
    for (idx = start_idx; idx < (p_msg_in->size - 1U); idx++)
    {
      if ((p_atp_ctxt->current_atcmd.id == _AT_QIRD) &&
          (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Payload) &&
          (BG96_ctxt.socket_ctxt.socket_RxData_state != SocketRxDataState_finished))
      {
        PrintDBG("receiving socket data (real size=%d)", SocketHeaderRX_getSize())
        element_infos->str_start_idx = 0U;
        element_infos->str_end_idx = (uint16_t) BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        element_infos->str_size = (uint16_t) BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_finished;
        return (ATENDMSG_YES);
      }
    }
    /* ###########################  END CUSTOMIZATION PART  ########################### */
  }

  element_infos->str_start_idx = *p_parseIndex;
  element_infos->str_end_idx = *p_parseIndex;
  element_infos->str_size = 0U;

  /* reach limit of input buffer ? (empty message received) */
  if (*p_parseIndex >= p_msg_in->size)
  {
    return (ATENDMSG_YES);
  }

  /* extract parameter from message */
  do
  {
    switch (p_msg_in->buffer[*p_parseIndex])
    {
      /* ###########################  START CUSTOMIZATION PART  ########################### */
      /* ----- test separators ----- */
      case ' ':
        if ((p_atp_ctxt->current_atcmd.id == _AT_CGDATA) || (p_atp_ctxt->current_atcmd.id == _ATD))
        {
          /* specific case for BG96 which answer CONNECT <value> even when ATX0 sent before */
          exit_loop = 1U;
        }
        else
        {
          element_infos->str_end_idx = *p_parseIndex;
          element_infos->str_size++;
        }
        break;

      case ':':
      case ',':
        exit_loop = 1U;
        break;

      /* ----- test end of message ----- */
      case '\r':
        exit_loop = 1U;
        retval_msg_end_detected = ATENDMSG_YES;
        break;

      default:
        /* increment end position */
        element_infos->str_end_idx = *p_parseIndex;
        element_infos->str_size++;
        break;
        /* ###########################  END CUSTOMIZATION PART  ########################### */
    }

    /* increment index */
    (*p_parseIndex)++;

    /* reach limit of input buffer ? */
    if (*p_parseIndex >= p_msg_in->size)
    {
      exit_loop = 1U;
      retval_msg_end_detected = ATENDMSG_YES;
    }
  }
  while (!exit_loop);

  /* increase parameter rank */
  element_infos->param_rank = (element_infos->param_rank + 1U);

  return (retval_msg_end_detected);
}

at_action_rsp_t ATCustom_BG96_analyzeCmd(at_context_t *p_at_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_ERROR;

  PrintAPI("enter ATCustom_BG96_analyzeCmd()")

  /* search in LUT the ID corresponding to command received */
  if (ATSTATUS_OK != atcm_searchCmdInLUT(&BG96_ctxt, p_atp_ctxt, p_msg_in, element_infos))
  {
    /* if cmd_id not found in the LUT, may be it's a text line to analyze */

    /* 1st STEP: search in common modems commands

      NOTE: if this modem has a specific behaviour for one of the common commands, bypass this
      step and manage all command in the 2nd step
    */
    retval = atcm_check_text_line_cmd(&BG96_ctxt, p_at_ctxt, p_msg_in, element_infos);

    /* 2nd STEP: search in specific modems commands if not found at 1st STEP */
    if (retval == ATACTION_RSP_NO_ACTION)
    {
      switch (p_atp_ctxt->current_atcmd.id)
      {
        /* ###########################  START CUSTOMIZED PART  ########################### */
        case _AT_QIRD:
          if (fRspAnalyze_QIRD_data_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        case _AT_QISTATE:
          if (fRspAnalyze_QISTATE_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        /* ###########################  END CUSTOMIZED PART  ########################### */
        default:
          /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
          PrintDBG("receive an un-expected line... is it a text line ?")
          retval = ATACTION_RSP_IGNORED;
          break;
      }
    }

    /* we fall here when cmd_id not found in the LUT
    * 2 cases are possible:
    *  - this is a valid line: ATACTION_RSP_INTERMEDIATE
    *  - this is an invalid line: ATACTION_RSP_ERROR
    */
    return (retval);
  }

  /* cmd_id has been found in the LUT: determine next action */
  switch (element_infos->cmd_id_received)
  {
    case _AT_OK:
      if (p_atp_ctxt->current_SID == SID_CS_DATA_SUSPEND)
      {
        PrintINFO("MODEM SWITCHES TO COMMAND MODE")
        atcm_set_modem_data_mode(&BG96_ctxt, AT_FALSE);
      }
      if ((p_atp_ctxt->current_SID == SID_CS_POWER_ON) || (p_atp_ctxt->current_SID == SID_CS_RESET))
      {
        if (p_atp_ctxt->current_atcmd.id == _AT)
        {
          /* bg96_boot_synchro = AT_TRUE; */
          PrintDBG("modem synchro established")
        }
        if (p_atp_ctxt->current_atcmd.id == _ATE)
        {
          PrintDBG("Echo successfully disabled")
        }
      }
      if (p_atp_ctxt->current_SID == SID_CS_PING_IP_ADDRESS)
      {
        PrintDBG("this is a valid PING request")
        atcm_validate_ping_request(&BG96_ctxt);
      }
      retval = ATACTION_RSP_FRC_END;
      break;

    case _AT_RING:
    case _AT_NO_CARRIER:
    case _AT_NO_DIALTONE:
    case _AT_BUSY:
    case _AT_NO_ANSWER:
      /* VALUES NOT MANAGED IN CURRENT IMPLEMENTATION BECAUSE NOT EXPECTED */
      retval = ATACTION_RSP_ERROR;
      break;

    case _AT_CONNECT:
      PrintINFO("MODEM SWITCHES TO DATA MODE")
      atcm_set_modem_data_mode(&BG96_ctxt, AT_TRUE);
      retval = (at_action_rsp_t)(ATACTION_RSP_FLAG_DATA_MODE | ATACTION_RSP_FRC_END);
      break;

    /* ###########################  START CUSTOMIZATION PART  ########################### */
    case _AT_CEREG:
    case _AT_CREG:
    case _AT_CGREG:
      /* check if response received corresponds to the command we have send
      *  if not => this is an URC
      */
      if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      else
      {
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      break;

    case _AT_RDY_EVENT:
      PrintDBG(" MODEM READY TO RECEIVE AT COMMANDS")
      /* modem is ready */
      BG96_ctxt.persist.modem_at_ready = AT_TRUE;
      at_bool_t event_registered;
      event_registered = atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_BOOT);

      /* if we were waiting for this event, we can continue the sequence */
      if ((p_atp_ctxt->current_SID == SID_CS_POWER_ON) || (p_atp_ctxt->current_SID == SID_CS_RESET))
      {
        /* UNLOCK the WAIT EVENT : as there are still some commands to send after, use CONTINUE */
        BG96_ctxt.persist.modem_at_ready = AT_FALSE;
        retval = ATACTION_RSP_FRC_CONTINUE;
      }
      else
      {
        if (event_registered == AT_TRUE)
        {
          retval = ATACTION_RSP_URC_FORWARDED;
        }
        else
        {
          retval = ATACTION_RSP_URC_IGNORED;
        }
      }
      break;

    case _AT_POWERED_DOWN_EVENT:
      PrintDBG("MODEM POWERED DOWN EVENT DETECTED")
      if (atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_POWER_DOWN) == AT_TRUE)
      {
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      else
      {
        retval = ATACTION_RSP_URC_IGNORED;
      }
      break;

    case _AT_SOCKET_PROMPT:
      PrintINFO(" SOCKET PROMPT RECEIVED")
      /* if we were waiting for this event, we can continue the sequence */
      if (p_atp_ctxt->current_SID == SID_CS_SEND_DATA)
      {
        /* UNLOCK the WAIT EVENT */
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        retval = ATACTION_RSP_URC_IGNORED;
      }
      break;

    case _AT_SEND_OK:
      if (p_atp_ctxt->current_SID == SID_CS_SEND_DATA)
      {
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        retval = ATACTION_RSP_ERROR;
      }
      break;

    case _AT_SEND_FAIL:
      retval = ATACTION_RSP_ERROR;
      break;

    case _AT_QIURC:
      /* retval will be override in analyze of +QUIRC content
      *  indeed, QIURC can be considered as an URC or a normal msg (for DNS request)
      */
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case _AT_QIOPEN:
      /* now waiting for an URC  */
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case _AT_QIND:
      retval = ATACTION_RSP_URC_FORWARDED;
      break;

    case _AT_CFUN:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case _AT_CPIN:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case _AT_QCFG:
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case _AT_QUSIM:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case _AT_CPSMS:
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case _AT_CEDRXS:
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case _AT_CGEV:
      retval = ATACTION_RSP_URC_FORWARDED;
      break;

    case _AT_QPING:
      retval = ATACTION_RSP_URC_FORWARDED;
      break;

    /* ###########################  END CUSTOMIZATION PART  ########################### */

    case _AT:
      retval = ATACTION_RSP_IGNORED;
      break;

    case _AT_INVALID:
      retval = ATACTION_RSP_ERROR;
      break;

    case _AT_ERROR:
      /* ERROR does not contains parameters, call the analyze function explicity */
      retval = fRspAnalyze_Error_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
      break;

    case _AT_CME_ERROR:
    case _AT_CMS_ERROR:
      /* do the analyze here because will not be called by parser */
      retval = fRspAnalyze_Error_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
      break;

    default:
      /* check if response received corresponds to the command we have send
      *  if not => this is an ERROR
      */
      if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      else
      {
        PrintINFO("UNEXPECTED MESSAGE RECEIVED")
        retval = ATACTION_RSP_IGNORED;
      }
      break;
  }

  return (retval);
}

at_action_rsp_t ATCustom_BG96_analyzeParam(at_context_t *p_at_ctxt,
                                           IPC_RxMessage_t *p_msg_in,
                                           at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PrintAPI("enter ATCustom_BG96_analyzeParam()")

  /* analyse parameters of the command we received:
  * call the corresponding function from the LUT
  */
  retval = (atcm_get_CmdAnalyzeFunc(&BG96_ctxt, element_infos->cmd_id_received))(p_at_ctxt,
           &BG96_ctxt,
           p_msg_in,
           element_infos);

  return (retval);
}

/* function called to finalize an AT command */
at_action_rsp_t ATCustom_BG96_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter ATCustom_BG96_terminateCmd()")

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* additional tests */
  if (BG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
  {
    /* special case for SID_CS_SEND_DATA
    * indeed, this function is called when an AT cmd is finished
    * but for AT+QISEND, it is called a 1st time when prompt is received
    * and a second time when data have been sent.
    */
    if (p_atp_ctxt->current_SID != SID_CS_SEND_DATA)
    {
      /* reset socket_send_state */
      BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_No_Activity;
    }
  }

  if ((p_atp_ctxt->current_atcmd.id == _ATD) ||
      (p_atp_ctxt->current_atcmd.id == _ATO) ||
      (p_atp_ctxt->current_atcmd.id == _AT_CGDATA))
  {
    if (element_infos->cmd_id_received != _AT_CONNECT)
    {
      PrintErr("expected CONNECT not received !!!")
      return (ATACTION_RSP_ERROR);
    }
    else
    {
      /* force last command (no command can be sent in DATA mode) */
      p_atp_ctxt->is_final_cmd = 1U;
      PrintDBG("CONNECT received")
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */
  return (retval);
}

/* function called to finalize a SID */
at_status_t ATCustom_BG96_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATCustom_BG96_get_rsp()")

  /* prepare response for a SID - common part */
  retval = atcm_modem_get_rsp(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for a SID
  *  all specific behaviors for SID which are returning datas in rsp_buf have to be implemented here
  */
  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DNS_REQ:
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf, CSMT_DNS_REQ, sizeof(bg96_qiurc_dnsgip.hostIPaddr), (void *)bg96_qiurc_dnsgip.hostIPaddr) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;

    case SID_CS_POWER_ON:
    case SID_CS_RESET:
      display_user_friendly_mode_and_bands_config();
      break;

    default:
      break;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  /* reset SID context */
  atcm_reset_SID_context(&BG96_ctxt.SID_ctxt);

  /* reset socket context */
  atcm_reset_SOCKET_context(&BG96_ctxt);

  return (retval);
}

at_status_t ATCustom_BG96_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATCustom_BG96_get_urc()")

  /* prepare response for an URC - common part */
  retval = atcm_modem_get_urc(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for an URC
  *  all specific behaviors for an URC have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

at_status_t ATCustom_BG96_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter ATCustom_BG96_get_error()")

  /* prepare response when an error occured - common part */
  retval = atcm_modem_get_error(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /*  prepare response when an error occured
  *  all specific behaviors for an error have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/* Private function Definition -----------------------------------------------*/

/* ###########################  START CUSTOMIZATION PART  ########################### */
/* Build command functions ------------------------------------------------------- */
static at_status_t fCmdBuild_ATD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATD_BG96()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PrintINFO("Activate PDN (user cid = %d, modem cid = %d)", (uint8_t)current_conf_id, modem_cid)

    sprintf((char *)p_atp_ctxt->current_atcmd.params, "*99***%d#", modem_cid);

    /* sprintf((char *)p_atp_ctxt->current_atcmd.params , "*99#" ); */
  }
  return (retval);
}

static at_status_t fCmdBuild_CGSN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGSN_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* BG96 only supports EXECUTION form of CGSN */
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

static at_status_t fCmdBuild_QPOWD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QPOWD_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Immediatly Power Down */
    /* sprintf((char *)p_atp_ctxt->current_atcmd.params , "0"); */
    /* Normal Power Down */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "1");
  }

  return (retval);
}

static at_status_t fCmdBuild_QCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    AT_CHAR_t cmd_param1[16] = "\0";
    AT_CHAR_t cmd_param2[16] = "\0";
    AT_CHAR_t cmd_param3[16] = "\0";
    AT_CHAR_t cmd_param4[16] = "\0";
    AT_CHAR_t cmd_param5[16] = "\0";
    uint8_t  cmd_nb_params = 0U;

    if (BG96_QCFG_command_write == AT_TRUE)
    {
      /* BG96_AT_Commands_Manual_V2.0 */
      switch (BG96_QCFG_command_param)
      {
        case _QCFG_gprsattach:
          /* cmd_nb_params = 1U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_nwscanseq:
          cmd_nb_params = 2U;
          /* param 1 = scanseq */
          sprintf((char *)&cmd_param1, "0%lx", BG96_SCANSEQ);  /* print as hexa but without prefix, need to add 1st digit = 0*/
          /* param 2 = effect */
          sprintf((char *)&cmd_param2, "%d", 1);  /* 1 means take effect immediatly */
          break;
        case _QCFG_nwscanmode:
          cmd_nb_params = 2U;
          /* param 1 = scanmode */
          sprintf((char *)&cmd_param1, "%ld", BG96_SCANMODE);
          /* param 2 = effect */
          sprintf((char *)&cmd_param2, "%d", 1);  /* 1 means take effect immediatly */
          break;
        case _QCFG_iotopmode:
          cmd_nb_params = 2U;
          /* param 1 = iotopmode */
          sprintf((char *)&cmd_param1, "%ld", BG96_IOTOPMODE);
          /* param 2 = effect */
          sprintf((char *)&cmd_param2, "%d", 1);  /* 1 means take effect immediatly */
          break;
        case _QCFG_roamservice:
          /* cmd_nb_params = 2U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_band:
          cmd_nb_params = 4U;
          /* param 1 = gsmbandval */
          sprintf((char *)&cmd_param1, "%lx", BG96_BAND_GSM);
          /* param 2 = catm1bandval */
          sprintf((char *)&cmd_param2, "%llx", BG96_BAND_CAT_M1);
          /* param 3 = catnb1bandval */
          sprintf((char *)&cmd_param3, "%llx", BG96_BAND_CAT_NB1);
          /* param 4 = effect */
          sprintf((char *)&cmd_param4, "%d", 1);  /* 1 means take effect immediatly */
          break;
        case _QCFG_servicedomain:
          /* cmd_nb_params = 2U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_sgsn:
          /* cmd_nb_params = 1U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_msc:
          /* cmd_nb_params = 1U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_PDP_DuplicateChk:
          cmd_nb_params = 1U;
          /* param 1 = enable */
          sprintf((char *)&cmd_param1, "%d", BG96_PDP_DUPLICATECHK_ENABLE);
          break;
        case _QCFG_urc_ri_ring:
          /* cmd_nb_params = 5U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_urc_ri_smsincoming:
          /* cmd_nb_params = 2U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_urc_ri_other:
          /* cmd_nb_params = 2U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_signaltype:
          /* cmd_nb_params = 1U; */
          /* NOT IMPLEMENTED */
          break;
        case _QCFG_urc_delay:
          /* cmd_nb_params = 1U; */
          /* NOT IMPLEMENTED */
          break;
        default:
          break;
      }
    }

    if (cmd_nb_params == 5U)
    {
      /* command has 5 parameters (this is a WRITE command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s,%s,%s", BG96_QCFG_LUT[BG96_QCFG_command_param], cmd_param1, cmd_param2, cmd_param3, cmd_param4, cmd_param5);
    }
    else if (cmd_nb_params == 4U)
    {
      /* command has 4 parameters (this is a WRITE command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s,%s", BG96_QCFG_LUT[BG96_QCFG_command_param], cmd_param1, cmd_param2, cmd_param3, cmd_param4);
    }
    else if (cmd_nb_params == 3U)
    {
      /* command has 3 parameters (this is a WRITE command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s", BG96_QCFG_LUT[BG96_QCFG_command_param], cmd_param1, cmd_param2, cmd_param3);
    }
    else if (cmd_nb_params == 2U)
    {
      /* command has 2 parameters (this is a WRITE command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s", BG96_QCFG_LUT[BG96_QCFG_command_param], cmd_param1, cmd_param2);
    }
    else if (cmd_nb_params == 1U)
    {
      /* command has 1 parameters (this is a WRITE command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s", BG96_QCFG_LUT[BG96_QCFG_command_param], cmd_param1);
    }
    else
    {
      /* command has 0 parameters (this is a READ command) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"%s\"", BG96_QCFG_LUT[BG96_QCFG_command_param]);
    }
  }


  return (retval);
}

static at_status_t fCmdBuild_QINDCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QINDCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    switch (BG96_QINDCFG_command_param)
    {
      case _QINDCFG_csq:
        if (p_atp_ctxt->current_SID == SID_CS_SUSBCRIBE_NET_EVENT)
        {
          /* subscribe to CSQ URC event, do not save to nvram */
          sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"csq\",1,0");
        }
        else
        {
          /* unsubscribe to CSQ URC event, do not save to nvram */
          sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"csq\",0,0");
        }
        break;

      case _QINDCFG_all:
      case _QINDCFG_smsfull:
      case _QINDCFG_ring:
      case _QINDCFG_smsincoming:
      default:
        /* not implemented yet or error */
        break;
    }
  }

  return (retval);
}

static at_status_t fCmdBuild_CPSMS_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CPSMS_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf BG96 AT commands manual V2.0 (paragraph 6.7)
    AT+CPSMS=[<mode>[,<Requested_Periodic-RAU>[,<Requested_GPRS-READY-timer>[,<Requested_Periodic-TAU>[
    ,<Requested_Active-Time>]]]]]

    mode: 0 disable PSM, 1 enable PSM, 2 disable PSM and discard all params or reset to manufacturer values
    Requested_Periodic-RAU:
    Requested_GPRS-READY-timer:
    Requested_Periodic-TAU:
    Requested_Active-Time:

    exple:
    AT+CPSMS=1,,,”00000100”,”00001111”
    Set the requested T3412 value to 40 minutes, and set the requested T3324 value to 30 seconds
    */
#if (BG96_ENABLE_PSM == 1)
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "1,,,\"%s\",\"%s\"", BG96_CPSMS_REQ_PERIODIC_TAU, BG96_CPSMS_REQ_ACTIVE_TIME);
#else
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
#endif

  }

  return (retval);
}

static at_status_t fCmdBuild_CEDRXS_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CEDRXS_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf BG96 AT commands manual V2.0 (paragraph 6.7)
    AT+CEDRXS=[<mode>,[,<AcT-type>[,<Requested_eDRX_value>]]]

    mode: 0 disable e-I-DRX, 1 enable e-I-DRX, 2 enable e-I-DRX and URC +CEDRXP
    AcT-type: 0 Act not using e-I-DRX, 1 LTE cat.M1, 2 GSM, 3 UTRAN, 4 LTE, 5 LTE cat.NB1
    Requested_eDRX_value>:

    exple:
    AT+CEDRX=1,5,”0000”
    Set the requested e-I-DRX value to 5.12 second
    */
#if (BG96_ENABLE_E_I_DRX == 1)
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "1,%d,\"0000\"", BG96_CEDRXS_ACT_TYPE);
#else
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
#endif
  }

  return (retval);
}

static at_status_t fCmdBuild_QENG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QENG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf addendum for descirption of QENG AT command
     * AT+QENG=<celltype>
    */

    switch (BG96_QENG_command_param)
    {

      case _QENGcelltype_ServingCell:
        sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"servingcell\"");
        break;

      case _QENGcelltype_NeighbourCell:
        sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"neighbourcell\"");
        break;

      case _QENGcelltype_PSinfo:
        sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"psinfo\"");
        break;

      default:
        /* not implemented or supported */
        retval = ATSTATUS_ERROR;
        break;
    }
  }

  return (retval);
}

static at_status_t fCmdBuild_QICSGP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QICSGP_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf BG96 TCP/IP AT commands manual v1.0
    *  AT+QICSGP: Configure parameters of a TCP/IP context
    *  AT+QICSGP=<cid>[,<context_type>,<APN>[,<username>,<password>)[,<authentication>]]]
    *  - cid: context id (rang 1 to 16)
    *  - context_type: 1 for IPV4, 2 for IPV6
    *  - APN: string for access point name
    *  - username: string
    *  - password: string
    *  - authentication: 0 for NONE, 1 for PAP, 2 for CHAP, 3 for PAP or CHAP
    *
    * example: AT+QICSGP=1,1,"UNINET","","",1
    */

    if (bg96_QICGSP_config_command == AT_TRUE)
    {
      /* Write command is a config command */
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)

      uint8_t context_type_value;
      uint8_t authentication_value;

      /* convert context type to numeric value */
      switch (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type)
      {
        case CS_PDPTYPE_IP:
          context_type_value = 1U;
          break;
        case CS_PDPTYPE_IPV6:
        case CS_PDPTYPE_IPV4V6:
          context_type_value = 2U;
          break;

        default :
          context_type_value = 1U;
          break;
      }

      /*  authentication : 0,1,2 or 3 - cf modem AT cmd manuel - Use 0 for None */
      authentication_value = 0U;

      /* build command */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\",\"%s\",\"%s\",%d",
              modem_cid,
              context_type_value,
              p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn,
              p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username,
              p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.password,
              authentication_value
             );
    }
    else
    {
      /* Write command is a query command */
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
    }

  }

  return (retval);
}

static at_status_t fCmdBuild_QIACT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIACT_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d",  modem_cid);
  }

  return (retval);
}

static at_status_t fCmdBuild_QICFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QICFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

static at_status_t fCmdBuild_QIOPEN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIOPEN_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QIOPEN=<contextID>,<connectId>,<service_type>,<IP_address>/<domain_name>,<remote_port>[,
      *           <local_port>[,<access_mode>]]
      *
      * <contextID> is the PDP context ID
      */
      /* convert user cid (CS_PDN_conf_id_t) to PDP modem cid (value) */
      uint8_t pdp_modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, p_modem_ctxt->socket_ctxt.socket_info->conf_id);
      PrintDBG("user cid = %d, PDP modem cid = %d", (uint8_t)p_modem_ctxt->socket_ctxt.socket_info->conf_id, pdp_modem_cid)
      uint8_t _access_mode = 0U; /* 0=buffer acces mode, 1=direct push mode, 2=transparent access mode */

      if (p_atp_ctxt->current_SID == SID_CS_DIAL_COMMAND)
      {
        /* client mode: "TCP" of "UDP" */
        uint8_t _service_type_index = ((p_modem_ctxt->socket_ctxt.socket_info->protocol == CS_TCP_PROTOCOL) ? _QIOPENservicetype_TCP_Client : _QIOPENservicetype_UDP_Client);

        sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,%ld,\"%s\",\"%s\",%d,%d,%d",
                pdp_modem_cid,
                atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle),
                bg96_array_QIOPEN_service_type[_service_type_index],
                p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                p_modem_ctxt->socket_ctxt.socket_info->remote_port,
                p_modem_ctxt->socket_ctxt.socket_info->local_port,
                _access_mode);

        /* waiting for +QIOPEN now */
        bg96_waiting_qiopen = AT_TRUE;
      }
      /* QIOPEN for server mode (not supported yet)
      *  else if (curSID == ?) for server mode (corresponding to CDS_socket_listen)
      */
      else
      {
        /* error */
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

static at_status_t fCmdBuild_QICLOSE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QICLOSE_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QICLOSE=connectId>[,<timeout>]
      */
      uint32_t connID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%ld", connID);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }
  return (retval);
}

static at_status_t fCmdBuild_QISEND_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISEND_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /*IP AT Commands manual - LTE Module Series - V1.0
    * send data in command mode:
    * AT+QISEND=<connId>,<send_length><CR>
    * > ...DATA...
    *
    * DATA are sent using fCmdBuild_QISEND_WRITE_DATA_BG96()
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
            atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle),
            p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size
           );
  }

  return (retval);
}

static at_status_t fCmdBuild_QISEND_WRITE_DATA_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISEND_WRITE_DATA_BG96()")

  /* after having send AT+QISEND and prompt received, now send DATA */

  /* only for raw command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send != NULL)
    {
      uint32_t str_size = p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size;
      memcpy((void *)p_atp_ctxt->current_atcmd.params,
             (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send,
             str_size);

      /* set raw command size */
      p_atp_ctxt->current_atcmd.raw_cmd_size = str_size;
    }
    else
    {
      PrintErr("ERROR, send buffer is a NULL ptr !!!")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

static at_status_t fCmdBuild_QIRD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIRD_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
    {
      /* requesting socket data size (set length = 0) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%ld,0",
              atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle));
    }
    else if (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
    {
      uint32_t requested_data_size = 0U;
#if defined(USE_C2C_CS_ADAPT)
      /* read only the requested size */
      requested_data_size = p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size;
#else
      requested_data_size = p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size;
#endif /* USE_C2C_CS_ADAPT */

      /* requesting socket data with correct size */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
              atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle),
              requested_data_size);

      /* ready to start receive socket buffer */
      p_modem_ctxt->socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
    }
    else
    {
      PrintErr("Unexpected socket receiving state")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

static at_status_t fCmdBuild_QISTATE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISTATE_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+QISTATE=<query_type>,<connectID>
    *
    * <query_type> = 1
    */

    sprintf((char *)p_atp_ctxt->current_atcmd.params, "1,%ld",
            atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_cnx_infos->socket_handle));
  }

  return (retval);
}

static at_status_t fCmdBuild_CGDCONT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGDCONT_BG96()")

  /* normal case */
  retval = fCmdBuild_CGDCONT(p_atp_ctxt, p_modem_ctxt);

  return (retval);
}

static at_status_t fCmdBuild_QIDNSCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIDNSCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  1- Configure DNS server address for specified PDP context
    *  *  AT+QIDNSCFG=<contextID>,<pridnsaddr>[,<secdnsaddr>]
    *
    *  2- Query DNS server address of specified PDP context
    *  AT+QIDNSCFG=<contextID>
    *  response:
    *  +QIDNSCFG: <contextID>,<pridnsaddr>,<secdnsaddr>
    *
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    /* configure DNS server address for the specfied PDP context */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
            modem_cid,
            p_modem_ctxt->SID_ctxt.dns_request_infos->dns_conf.primary_dns_addr);
  }

  return (retval);
}

static at_status_t fCmdBuild_QIDNSGIP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIDNSGIP_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    * AT+QIDNSGIP=<contextID>,<hostname>
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
            modem_cid,
            p_modem_ctxt->SID_ctxt.dns_request_infos->dns_req.host_name);
  }
  return (retval);
}

static at_status_t fCmdBuild_QPING_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QPING_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  Ping a remote server:
    *  AT+QPING=<contextID>,<host<[,<timeout>[,<pingnum>]]
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",%d,%d",
            modem_cid,
            p_modem_ctxt->SID_ctxt.ping_infos.ping_params.host_addr,
            p_modem_ctxt->SID_ctxt.ping_infos.ping_params.timeout,
            p_modem_ctxt->SID_ctxt.ping_infos.ping_params.pingnum);
  }

  return (retval);
}

/* Analyze command functions ------------------------------------------------------- */
static at_action_rsp_t fRspAnalyze_CPIN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CPIN_BG96()")

  /*  Quectel BG96 AT Commands Manual V1.0
  *   analyze parameters for +CPIN
  *
  *   if +CPIN is not received after AT+CPIN request, it's an URC
  */
  if (p_atp_ctxt->current_atcmd.id == _AT_CPIN)
  {
    retval = fRspAnalyze_CPIN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
  }
  else
  {
    START_PARAM_LOOP()
    /* this is an URC */
    if (element_infos->param_rank == 2U)
    {
      PrintDBG("URC +CPIN received")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

static at_action_rsp_t fRspAnalyze_CFUN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_modem_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CFUN_BG96()")

  /*  Quectel BG96 AT Commands Manual V1.0
  *   analyze parameters for +CFUN
  *
  *   if +CFUN is received, it's an URC
  */

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PrintDBG("URC +CFUN received")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIND_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIND_BG96()")

  at_bool_t bg96_current_qind_is_csq = AT_FALSE;
  /* FOTA infos */
  at_bool_t bg96_current_qind_is_fota = AT_FALSE;
  uint8_t bg96_current_qind_fota_action = 0U; /* 0: ignored FOTA action , 1: FOTA start, 2: FOTA end  */

  /*  Quectel BG96 AT Commands Manual V1.0
  *   analyze parameters for +QIND
  *
  *   it's an URC
  */

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qind_is_csq = AT_FALSE;

    PrintDBG("URC +QIND received")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      memcpy((void *)&line[0],
             (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
             (size_t) element_infos->str_size);
    }
    else
    {
      PrintErr("line exceed maximum size, line ignored...")
      return (ATACTION_RSP_IGNORED);
    }

    /* extract value and compare it to expected value */
    if ((char *) strstr((const char *)&line[0], "csq") != NULL)
    {
      PrintDBG("SIGNAL QUALITY INFORMATION")
      bg96_current_qind_is_csq = AT_TRUE;
    }
    else if ((char *) strstr((const char *)&line[0], "FOTA") != NULL)
    {
      PrintDBG("URC FOTA infos received")
      bg96_current_qind_is_fota = AT_TRUE;
    }
    else
    {
      retval = ATACTION_RSP_URC_IGNORED;
      PrintDBG("QIND info not managed: urc ignored")
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    if (bg96_current_qind_is_csq == AT_TRUE)
    {
      uint32_t rssi = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ rssi=%ld", rssi)
      p_modem_ctxt->persist.urc_avail_signal_quality = AT_TRUE;
      p_modem_ctxt->persist.signal_quality.rssi = (uint8_t)rssi;
      p_modem_ctxt->persist.signal_quality.ber = 99U; /* in case ber param is not present */
    }
    else if (bg96_current_qind_is_fota == AT_TRUE)
    {
      AT_CHAR_t line[32] = {0};
      /* copy element to line for parsing */
      if (element_infos->str_size <= 32U)
      {
        memcpy((void *)&line[0],
               (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
               (size_t) element_infos->str_size);
        /* WARNING KEEP ORDER UNCHANGED (END comparison has to be after HTTPEND and FTPEND) */
        if ((char *) strstr((const char *)&line[0], "FTPSTART") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((char *) strstr((const char *)&line[0], "FTPEND") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((char *) strstr((const char *)&line[0], "HTTPSTART") != NULL)
        {
          PrintINFO("URC FOTA start detected !")
          bg96_current_qind_fota_action = 1U;
          if (atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_FOTA_START) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        else if ((char *) strstr((const char *)&line[0], "HTTPEND") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((char *) strstr((const char *)&line[0], "START") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((char *) strstr((const char *)&line[0], "END") != NULL)
        {
          PrintINFO("URC FOTA end detected !")
          bg96_current_qind_fota_action = 2U;
          if (atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_FOTA_END) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        else
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
      }
      else
      {
        PrintErr("line exceed maximum size, line ignored...")
        return (ATACTION_RSP_IGNORED);
      }
    }
    else
    {
      /* ignore */
    }

  }
  else if (element_infos->param_rank == 4U)
  {
    if (bg96_current_qind_is_csq == AT_TRUE)
    {
      uint32_t ber = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ ber=%ld", ber)
      p_modem_ctxt->persist.signal_quality.ber = (uint8_t)ber;
    }
    else if (bg96_current_qind_is_fota == AT_TRUE)
    {
      if (bg96_current_qind_fota_action == 1U)
      {
        /* FOTA END status
         * parameter ignored for the moment
         */
        retval = ATACTION_RSP_URC_IGNORED;
      }
    }
    else
    {
      /* ignored */
    }
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIACT_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)

{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIACT_BG96()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    /* Answer to AT+QIACT?
    *  Returns the list of current activated contexts and their IP addresses
    *  format: +QIACT: <cid>,<context_state>,<context_type>,[,<IP_address>]
    *
    *  where:
    *  <cid>: context ID, range is 1 to 16
    *  <context_state>: 0 for deactivated, 1 for activated
    *  <context_type>: 1 for IPV4, 2 for IPV6
    *  <IP_address>: string type
    */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* analyze <cid> */
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIACT cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if (element_infos->param_rank == 3U)
    {
      /* analyze <context_state> */
      uint32_t context_state = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIACT context_state=%ld", context_state)

      /* If we are trying to activate a PDN and we see that it is already active, do not request the activation to avoid an error */
      if ((p_atp_ctxt->current_SID == SID_CS_ACTIVATE_PDN) && (context_state == 1U))
      {
        /* is it the required PDN to activate ? */
        CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
        uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
        if (p_modem_ctxt->CMD_ctxt.modem_cid == modem_cid)
        {
          PrintDBG("Modem CID to activate (%d) is already activated", modem_cid)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
          bg96_pdn_already_active = AT_TRUE;
#endif /* USE_SOCKETS_TYPE */
        }
      }
    }
    else if (element_infos->param_rank == 4U)
    {
      /* analyze <context_type> */
      uint32_t context_type = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIACT context_type=%ld", context_type)
    }
    else if (element_infos->param_rank == 5U)
    {
      /* analyze <IP_address> */
      csint_ip_addr_info_t  ip_addr_info;
      memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrive IP address value */
      memcpy((void *) & (ip_addr_info.ip_addr_value),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
      PrintDBG("+QIACT addr=%s", (char *)&ip_addr_info.ip_addr_value)

      /* determine IP address type (IPv4 or IPv6): count number of '.' */
      AT_CHAR_t *pTmp = (AT_CHAR_t *)&ip_addr_info.ip_addr_value;
      uint8_t count_dots = 0U;
      while (pTmp != NULL)
      {
        pTmp = (AT_CHAR_t *)strchr((char *)pTmp, '.');
        if (pTmp)
        {
          pTmp++;
          count_dots++;
        }
      }
      PrintDBG("+QIACT nbr of points in address %d", count_dots)
      if (count_dots == 3U)
      {
        ip_addr_info.ip_addr_type = CS_IPAT_IPV4;
      }
      else
      {
        ip_addr_info.ip_addr_type = CS_IPAT_IPV6;
      }

      /* save IP address infos in modem_cid_table */
      atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
    }
    else
    {
      /* other parameters ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_modem_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QCFG_BG96()")

  /* memorize which is current QCFG command received */
  ATCustom_BG96_QCFG_function_t bg96_current_qcfg_cmd = _QCFG_unknown;

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qcfg_cmd = _QCFG_unknown;

    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      memcpy((void *)&line[0],
             (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
             (size_t) element_infos->str_size);
    }
    else
    {
      PrintErr("line exceed maximum size, line ignored...")
      return (ATACTION_RSP_IGNORED);
    }

    /* extract value and compare it to expected value */
    if ((char *) strstr((const char *)&line[0], "nwscanseq") != NULL)
    {
      PrintDBG("+QCFG nwscanseq infos received")
      bg96_current_qcfg_cmd = _QCFG_nwscanseq;
    }
    else if ((char *) strstr((const char *)&line[0], "nwscanmode") != NULL)
    {
      PrintDBG("+QCFG nwscanmode infos received")
      bg96_current_qcfg_cmd = _QCFG_nwscanmode;
    }
    else if ((char *) strstr((const char *)&line[0], "iotopmode") != NULL)
    {
      PrintDBG("+QCFG iotopmode infos received")
      bg96_current_qcfg_cmd = _QCFG_iotopmode;
    }
    else if ((char *) strstr((const char *)&line[0], "band") != NULL)
    {
      PrintDBG("+QCFG band infos received")
      bg96_current_qcfg_cmd = _QCFG_band;
    }
    else
    {
      PrintErr("+QCFDG field not managed")
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case _QCFG_nwscanseq:
        bg96_mode_and_bands_config.nw_scanseq = (ATCustom_BG96_QCFGscanseq_t) ATutil_convertHexaStringToInt64(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        break;
      case _QCFG_nwscanmode:
        bg96_mode_and_bands_config.nw_scanmode = (ATCustom_BG96_QCFGscanmode_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        break;
      case _QCFG_iotopmode:
        bg96_mode_and_bands_config.iot_op_mode = (ATCustom_BG96_QCFGiotopmode_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        break;
      case _QCFG_band:
        bg96_mode_and_bands_config.gsm_bands = (ATCustom_BG96_QCFGbandGSM_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        /* display_decoded_GSM_bands(bg96_mode_and_bands_config.gsm_bands); */
        break;
      default:
        break;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case _QCFG_band:
        bg96_mode_and_bands_config.CatM1_bands = (ATCustom_BG96_QCFGbandCatM1_t) ATutil_convertStringToInt64(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        /* display_decoded_CatM1_bands(bg96_mode_and_bands_config.CatM1_bands); */
        break;
      default:
        break;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case _QCFG_band:
        bg96_mode_and_bands_config.CatNB1_bands = (ATCustom_BG96_QCFGbandCatNB1_t) ATutil_convertStringToInt64(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        /* display_decoded_CatNB1_bands(bg96_mode_and_bands_config.CatNB1_bands); */
        break;
      default:
        break;
    }
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QINDCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QINDCFG_BG96()")

  /* not implemented yet */

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIOPEN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                               IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIOPEN_BG96()")

  uint32_t bg96_current_qiopen_connectId = 0U;

  /* are we waiting for QIOPEN ? */
  if (bg96_waiting_qiopen == AT_TRUE)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t connectID;
      connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      bg96_current_qiopen_connectId = connectID;
      bg96_current_qiopen_socket_connected = 0U;
    }
    else if (element_infos->param_rank == 3U)
    {
      uint32_t err_value;
      err_value = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);

      /* compare QIOPEN connectID with the value requested by user (ie in current SID)
      *  and check if err=0
      */
      if ((bg96_current_qiopen_connectId == atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle)) &&
          (err_value == 0U))
      {
        PrintINFO("socket (connectId=%ld) opened", bg96_current_qiopen_connectId)
        bg96_current_qiopen_socket_connected = 1U;
        bg96_waiting_qiopen = AT_FALSE;
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        if (err_value != 0U)
        {
          PrintErr("+QIOPEN returned error #%ld", err_value)
        }
        else
        {
          PrintErr("+QIOPEN problem")
        }
        retval = ATACTION_RSP_ERROR;
      }
    }
    else
    {
      /* other parameters ignored */
    }
    END_PARAM_LOOP()
  }
  else
  {
    PrintINFO("+QIOPEN not expected, ignore it")
  }
  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIURC_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIURC_BG96()")

  /* memorize which is current QIURC received */
  ATCustom_BG96_QIURC_function_t bg96_current_qiurc_ind = _QIURC_unknown;

  /*IP AT Commands manual - LTE Module Series - V1.0
  * URC
  * +QIURC:"closed",<connectID> : URC of connection closed
  * +QIURC:"recv",<connectID> : URC of incoming data
  * +QIURC:"incoming full" : URC of incoming connection full
  * +QIURC:"incoming",<connectID> ,<serverID>,<remoteIP>,<remote_port> : URC of incoming connection
  * +QIURC:"pdpdeact",<contextID> : URC of PDP deactivation
  *
  * for DNS request:
  * header: +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
  * infos:  +QIURC: "dnsgip",<hostIPaddr>]
  */

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qiurc_ind = _QIURC_unknown;

    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      memcpy((void *)&line[0],
             (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
             (size_t) element_infos->str_size);
    }
    else
    {
      PrintErr("line exceed maximum size, line ignored...")
      return (ATACTION_RSP_IGNORED);
    }

    /* extract value and compare it to expected value */
    if ((char *) strstr((const char *)&line[0], "closed") != NULL)
    {
      PrintDBG("+QIURC closed info received")
      bg96_current_qiurc_ind = _QIURC_closed;
    }
    else if ((char *) strstr((const char *)&line[0], "recv") != NULL)
    {
      PrintDBG("+QIURC recv info received")
      bg96_current_qiurc_ind = _QIURC_recv;
    }
    else if ((char *) strstr((const char *)&line[0], "incoming full") != NULL)
    {
      PrintDBG("+QIURC incoming full info received")
      bg96_current_qiurc_ind = _QIURC_incoming_full;
    }
    else if ((char *) strstr((const char *)&line[0], "incoming") != NULL)
    {
      PrintDBG("+QIURC incoming info received")
      bg96_current_qiurc_ind = _QIURC_incoming;
    }
    else if ((char *) strstr((const char *)&line[0], "pdpdeact") != NULL)
    {
      PrintDBG("+QIURC pdpdeact info received")
      bg96_current_qiurc_ind = _QIURC_pdpdeact;
    }
    else if ((char *) strstr((const char *)&line[0], "dnsgip") != NULL)
    {
      PrintDBG("+QIURC dnsgip info received")
      bg96_current_qiurc_ind = _QIURC_dnsgip;
      if (p_atp_ctxt->current_SID != SID_CS_DNS_REQ)
      {
        /* URC not expected */
        retval = ATACTION_RSP_URC_IGNORED;
      }
    }
    else
    {
      PrintErr("+QIURC field not managed")
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    uint32_t connectID, contextID;
    socket_handle_t sockHandle;

    switch (bg96_current_qiurc_ind)
    {
      case _QIURC_recv:
        /* <connectID> */
        connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connectID);
        atcm_socket_set_urc_data_pending(p_modem_ctxt, sockHandle);
        PrintDBG("+QIURC received data for connId=%ld (socket handle=%d)", connectID, sockHandle)
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case _QIURC_closed:
        /* <connectID> */
        connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connectID);
        atcm_socket_set_urc_closed_by_remote(p_modem_ctxt, sockHandle);
        PrintDBG("+QIURC closed for connId=%ld (socket handle=%d)", connectID, sockHandle)
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case _QIURC_incoming:
        /* <connectID> */
        connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        PrintDBG("+QIURC incoming for connId=%ld", connectID)
        break;

      case _QIURC_pdpdeact:
        /* <contextID> */
        contextID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        PrintDBG("+QIURC pdpdeact for contextID=%ld", contextID)
        /* Need to inform  upper layer if pdn event URC has been subscribed
         * apply same treatment than CGEV NW PDN DEACT
        */
        p_modem_ctxt->persist.pdn_event.conf_id = atcm_get_configID_for_modem_cid(&p_modem_ctxt->persist, (uint8_t)contextID);
        /* Indicate that an equivalent to +CGEV URC has been received */
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_NW;
        p_modem_ctxt->persist.pdn_event.event_scope = CGEV_EVENT_SCOPE_PDN;
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DEACTIVATION;
        p_modem_ctxt->persist.urc_avail_pdn_event = AT_TRUE;
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case _QIURC_dnsgip:
        /* <err> or <hostIPaddr>]> */
        if (bg96_qiurc_dnsgip.wait_header == AT_TRUE)
        {
          /* <err> expected */
          bg96_qiurc_dnsgip.error = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PrintDBG("+QIURC dnsgip with error=%ld", bg96_qiurc_dnsgip.error)
          if (bg96_qiurc_dnsgip.error != 0U)
          {
            /* Error when trying to get host address */
            bg96_qiurc_dnsgip.finished = AT_TRUE;
            retval = ATACTION_RSP_ERROR;
          }
        }
        else
        {
          /* <hostIPaddr> expected
          *  with the current implementation, in case of many possible host IP address, we use
          *  the last one received
          */
          memcpy((void *)bg96_qiurc_dnsgip.hostIPaddr,
                 (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                 (size_t) element_infos->str_size);
          PrintDBG("+QIURC dnsgip Host address #%ld =%s", bg96_qiurc_dnsgip.ip_count, bg96_qiurc_dnsgip.hostIPaddr)
          bg96_qiurc_dnsgip.ip_count--;
          if (bg96_qiurc_dnsgip.ip_count == 0U)
          {
            /* all expected URC have been reecived */
            bg96_qiurc_dnsgip.finished = AT_TRUE;
            /* last param */
            retval = ATACTION_RSP_FRC_END;
          }
          else
          {
            retval = ATACTION_RSP_IGNORED;
          }
        }
        break;

      case _QIURC_incoming_full:
      default:
        /* no parameter expected */
        PrintErr("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    switch (bg96_current_qiurc_ind)
    {
      case _QIURC_incoming:
        /* <serverID> */
        PrintDBG("+QIURC incoming for serverID=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
        break;

      case _QIURC_dnsgip:
        /* <IP_count> */
        if (bg96_qiurc_dnsgip.wait_header == AT_TRUE)
        {
          /* <bg96_qiurc_dnsgip.ip_count> expected */
          bg96_qiurc_dnsgip.ip_count = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PrintDBG("+QIURC dnsgip IP count=%ld", bg96_qiurc_dnsgip.ip_count)
          if (bg96_qiurc_dnsgip.ip_count == 0U)
          {
            /* No host address available */
            bg96_qiurc_dnsgip.finished = AT_TRUE;
            retval = ATACTION_RSP_ERROR;
          }
        }
        break;

      case _QIURC_recv:
      case _QIURC_closed:
      case _QIURC_pdpdeact:
      case _QIURC_incoming_full:
      default:
        /* no parameter expected */
        PrintErr("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    AT_CHAR_t remoteIP[32] = {0};

    switch (bg96_current_qiurc_ind)
    {
      case _QIURC_incoming:
        /* <remoteIP> */
        memcpy((void *)&remoteIP[0],
               (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
               (size_t) element_infos->str_size);
        PrintDBG("+QIURC remoteIP for remoteIP=%s", remoteIP)
        break;

      case _QIURC_dnsgip:
        /* <DNS_ttl> */
        if (bg96_qiurc_dnsgip.wait_header == AT_TRUE)
        {
          /* <bg96_qiurc_dnsgip.ttl> expected */
          bg96_qiurc_dnsgip.ttl = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PrintDBG("+QIURC dnsgip time-to-live=%ld", bg96_qiurc_dnsgip.ttl)
          /* no error, now waiting for URC with IP address */
          bg96_qiurc_dnsgip.wait_header = AT_FALSE;
        }
        break;

      case _QIURC_recv:
      case _QIURC_closed:
      case _QIURC_pdpdeact:
      case _QIURC_incoming_full:
      default:
        /* no parameter expected */
        PrintErr("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    switch (bg96_current_qiurc_ind)
    {
      case _QIURC_incoming:
        /* <remote_port> */
        PrintDBG("+QIURC incoming for remote_port=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case _QIURC_recv:
      case _QIURC_closed:
      case _QIURC_pdpdeact:
      case _QIURC_incoming_full:
      case _QIURC_dnsgip:
      default:
        /* no parameter expected */
        PrintErr("parameter not expected for this URC message")
        break;
    }
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIRD_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIRD_BG96()")

  /*IP AT Commands manual - LTE Module Series - V1.0
  *
  * Received after having send AT+QIRD=<connectID>[,<read_length>]
  *
  * if <read_length> was present and equal to 0, it was a request to get status:
  * +QIRD:<total_receive_length>,<have_read_length>,<unread_length>
  *
  * if <read_length> was absent or != 0
  * +QIRD:<read_actual_length><CR><LF><data>
  *
  */
  if (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* <total_receive_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintINFO("+QIRD: total_receive_length = %ld", buff_in)
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <have_read_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintINFO("+QIRD: have_read_length = %ld", buff_in)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <unread_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintINFO("+QIRD: unread_length = %ld", buff_in)
      /* this is the size we have to request to read */
      p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size = buff_in;
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }
  else  if (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
  {
    START_PARAM_LOOP()
    /* Receiving socket DATA HEADER which include data size received */
    if (element_infos->param_rank == 2U)
    {
      /* <read_actual_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIRD: received data size = %ld", buff_in)
      /* NOTE !!! the size is purely informative in current implementation
      *  indeed, due to real time constraints, the socket data header is analyzed directly in ATCustom_BG96_checkEndOfMsgCallback()
      */
      /* data header analyzed, ready to analyze data payload */
      BG96_ctxt.socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Payload;
    }
    END_PARAM_LOOP()
  }
  else
  {
    PrintErr("+QIRD: should not fall here")
  }

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QIRD_data_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIRD_data_BG96()")

  PrintDBG("DATA received: size=%ld vs %d", p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size, element_infos->str_size)
  if (p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv != NULL)
  {
    /* recopy data to client buffer */
    memcpy((void *)p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv,
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t) element_infos->str_size);
    p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size = element_infos->str_size;
  }
  else
  {
    PrintErr("ERROR, receive buffer is a NULL ptr !!!")
    retval = ATACTION_RSP_ERROR;
  }

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QISTATE_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QISTATE_BG96()")

  at_bool_t bg96_qistate_for_requested_socket = AT_FALSE;

  /* format:
  * +QISTATE:<connectID>,<service_type>,<IP_adress>,<remote_port>,<local_port>,<socket_state>,<contextID>,<serverID>,<access_mode>,<AT_port>
  *
  * where:
  *
  * exple: +QISTATE: 0,“TCP”,“220.180.239.201”,8705,65514,2,1,0,0,“usbmodem”
  */

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* <connId> */
    uint32_t connId = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    socket_handle_t sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connId);
    /* if this connection ID corresponds to requested socket handle, we will report the following infos */
    if (sockHandle == p_modem_ctxt->socket_ctxt.socket_cnx_infos->socket_handle)
    {
      bg96_qistate_for_requested_socket = AT_TRUE;
    }
    else
    {
      bg96_qistate_for_requested_socket = AT_FALSE;
    }
    PrintDBG("+QISTATE: <connId>=%ld (requested=%d)", connId, ((bg96_qistate_for_requested_socket == AT_TRUE) ? 1 : 0))
  }
  else if (element_infos->param_rank == 3U)
  {
    /* <service_type> */
    AT_CHAR_t _ServiceType[16] = {0};
    memcpy((void *)&_ServiceType[0],
           (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
           (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <service_type>=%s", _ServiceType)
  }
  else if (element_infos->param_rank == 4U)
  {
    /* <IP_adress> */
    AT_CHAR_t remIP[MAX_IP_ADDR_SIZE];
    memset((void *)remIP, 0, MAX_IP_ADDR_SIZE);
    memcpy((void *)&remIP[0],
           (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
           (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <remote IP_adress>=%s", remIP)
    if (bg96_qistate_for_requested_socket == AT_TRUE)
    {
      uint16_t src_idx;
      uint16_t dest_idx = 0U;
      for (src_idx = element_infos->str_start_idx; src_idx <= element_infos->str_end_idx; src_idx++)
      {
        if (p_msg_in->buffer[src_idx] != 0x22U) /* remove quotes from the string */
        {
          p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_ip_addr_value[dest_idx] = p_msg_in->buffer[src_idx];
          dest_idx++;
        }
      }
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    /* <remote_port> */
    uint32_t remPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <remote_port>=%ld", remPort)
    if (bg96_qistate_for_requested_socket == AT_TRUE)
    {
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_port = (uint16_t) remPort;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    /* <local_port> */
    uint32_t locPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <local_port>=%ld", locPort)
    if (bg96_qistate_for_requested_socket == AT_TRUE)
    {
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_port = (uint16_t) locPort;
    }
  }
  else if (element_infos->param_rank == 7U)
  {
    /*<socket_state> */
    uint32_t sockState = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <socket_state>=%ld", sockState)
  }
  else if (element_infos->param_rank == 8U)
  {
    /* <contextID> */
    uint32_t contextID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <contextID>=%ld", contextID)
  }
  else if (element_infos->param_rank == 9U)
  {
    /* <serverID> */
    uint32_t serverID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <serverID>=%ld", serverID)
  }
  else if (element_infos->param_rank == 10U)
  {
    /* <access_mode> */
    uint32_t access_mode = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <access_mode>=%ld", access_mode)
  }
  else if (element_infos->param_rank == 11U)
  {
    /* <AT_port> */
    AT_CHAR_t _ATport[16] = {0};
    memcpy((void *)&_ATport[0],
           (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
           (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <AT_port>=%s", _ATport)
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_QPING_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_URC_FORWARDED;
  PrintAPI("enter fRspAnalyze_QPING_BG96()")

  /* intermediate ping response format:
  * +QPING: <result>[,<IP_address>,<bytes>,<time>,<ttl>]
  *
  * last ping reponse format:
  * +QPING: <finresult>[,<sent>,<rcvd>,<lost>,<min>,<max>,<avg>]
  */

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* new ping response: clear ping response structure */
    clear_ping_resp_struct(p_modem_ctxt);

    /* <result> or <finresult> */
    uint32_t result = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("Ping result = %ld", result)

    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_TRUE;
    p_modem_ctxt->persist.ping_resp_urc.index++;
    p_modem_ctxt->persist.ping_resp_urc.ping_status = (result == 0U) ? CELLULAR_OK : CELLULAR_ERROR;
  }
  else if (element_infos->param_rank == 3U)
  {
    /* check if this is an intermediate report or the final report */
    if (p_msg_in->buffer[element_infos->str_start_idx] == 0x22U)
    {
      /* this is an IP address: intermediate ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_TRUE)
      {
        PrintDBG("intermediate ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_FALSE;
      }
    }
    else
    {
      /* this is not an IP adress: final ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_FALSE)
      {
        PrintDBG("final ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_TRUE;
      }
    }

    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <IP_address> */
      AT_CHAR_t pingIP[MAX_IP_ADDR_SIZE];
      memset((void *)pingIP, 0, MAX_IP_ADDR_SIZE);
      memcpy((void *)&pingIP[0],
             (void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
             (size_t) element_infos->str_size);
      PrintDBG("+QIPING: <ping IP_adress>=%s", pingIP)
      uint16_t src_idx, dest_idx = 0U;
      for (src_idx = element_infos->str_start_idx; src_idx <= element_infos->str_end_idx; src_idx++)
      {
        if (p_msg_in->buffer[src_idx] != 0x22U) /* remove quotes from the string */
        {
          p_modem_ctxt->persist.ping_resp_urc.ping_addr[dest_idx] = p_msg_in->buffer[src_idx];
          dest_idx++;
        }
      }
    }
    else
    {
      /* <sent> */
      uint32_t packets_sent = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_sent = (uint8_t)packets_sent;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <bytes> */
      uint32_t bytes = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                 element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.bytes = (uint16_t)bytes;
    }
    else
    {
      /* <rcvd> */
      uint32_t packets_rcvd = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_rcvd = (uint8_t)packets_rcvd;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <time>*/
      uint32_t timeval = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.time = (uint16_t)timeval;
    }
    else
    {
      /* <lost> */
      uint32_t packets_lost = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_lost = (uint8_t)packets_lost;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <ttl>*/
      uint32_t ttl = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.ttl = (uint16_t)ttl;
    }
    else
    {
      /* <min_time>*/
      uint32_t min_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.min_time = (uint16_t)min_time;
    }
  }
  else if (element_infos->param_rank == 7U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <max_time>*/
      uint32_t max_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.max_time = (uint16_t)max_time;
    }
  }
  else if (element_infos->param_rank == 8U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <avg_time>*/
      uint32_t avg_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.avg_time = (uint16_t)avg_time;
    }
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

static at_action_rsp_t fRspAnalyze_Error_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PrintAPI("enter fRspAnalyze_Error_BG96()")

  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DIAL_COMMAND:
      /* in case of error during socket connection,
      * release the modem CID for this socket_handle
      */
      atcm_socket_release_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      break;

    default:
      /* nothing to do */
      break;
  }

  /* analyze Error for BG96 */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case _AT_CREG:
    case _AT_CGREG:
    case _AT_CEREG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case _AT_CPSMS:
    case _AT_CEDRXS:
    case _AT_QNWINFO:
    case _AT_QENG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case _AT_CGDCONT:
      if (p_atp_ctxt->current_SID == SID_CS_INIT_MODEM)
      {
        /* error is ignored in this case because this cmd is only informative */
        retval = ATACTION_RSP_FRC_END;
      }
      break;

    default:
      retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      break;
  }

  return (retval);
}

/* -------------------------------------------------------------------------- */
static void reinitSyntaxAutomaton_bg96(void)
{
  BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

static void reset_variables_bg96(void)
{
  /* Set default values of BG96 specific variables after SWITCH ON or RESET */
  /* bg96_boot_synchro = AT_FALSE; */
  bg96_mode_and_bands_config.nw_scanseq = 0xFFFFFFFFU;
  bg96_mode_and_bands_config.nw_scanmode = 0xFFFFFFFFU;
  bg96_mode_and_bands_config.iot_op_mode = 0xFFFFFFFFU;
  bg96_mode_and_bands_config.gsm_bands = 0xFFFFFFFFU;
  bg96_mode_and_bands_config.CatM1_bands = 0xFFFFFFFFFFFFFFFFU;
  bg96_mode_and_bands_config.CatNB1_bands = 0xFFFFFFFFFFFFFFFFU;
}

static void init_bg96_qiurc_dnsgip(void)
{
  bg96_qiurc_dnsgip.finished = AT_FALSE;
  bg96_qiurc_dnsgip.wait_header = AT_TRUE;
  bg96_qiurc_dnsgip.error = 0U;
  bg96_qiurc_dnsgip.ip_count = 0U;
  bg96_qiurc_dnsgip.ttl = 0U;
  memset((void *)bg96_qiurc_dnsgip.hostIPaddr, 0, MAX_SIZE_IPADDR);
}

static void display_decoded_GSM_bands(uint32_t gsm_bands)
{
  PrintINFO("GSM BANDS config = 0x%lx", gsm_bands)

  if ((gsm_bands & _QCFGbandGSM_900) != 0U)
  {
    PrintINFO("GSM_900")
  }
  if ((gsm_bands & _QCFGbandGSM_1800) != 0U)
  {
    PrintINFO("GSM_1800")
  }
  if ((gsm_bands & _QCFGbandGSM_850) != 0U)
  {
    PrintINFO("GSM_850")
  }
  if ((gsm_bands & _QCFGbandGSM_1900) != 0U)
  {
    PrintINFO("GSM_1900")
  }
}

static void display_decoded_CatM1_bands(uint64_t CatM1_bands)
{
  PrintINFO("Cat.M1 BANDS config = 0x%llx", CatM1_bands)

  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B1) != 0U)
  {
    PrintINFO("CatM1_B1")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B2) != 0U)
  {
    PrintINFO("CatM1_B2")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B3) != 0U)
  {
    PrintINFO("CatM1_B3")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B4) != 0U)
  {
    PrintINFO("CatM1_B4")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B5) != 0U)
  {
    PrintINFO("CatM1_B5")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B8) != 0U)
  {
    PrintINFO("CatM1_B8")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B12) != 0U)
  {
    PrintINFO("CatM1_B12")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B13) != 0U)
  {
    PrintINFO("CatM1_B13")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B18) != 0U)
  {
    PrintINFO("CatM1_B18")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B19) != 0U)
  {
    PrintINFO("CatM1_B19")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B20) != 0U)
  {
    PrintINFO("CatM1_B20")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B26) != 0U)
  {
    PrintINFO("CatM1_B26")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B28) != 0U)
  {
    PrintINFO("CatM1_B28")
  }
  if ((CatM1_bands & (unsigned long long)_QCFGbandCatM1_B39) != 0U)
  {
    PrintINFO("CatM1_B39")
  }
}

static void display_decoded_CatNB1_bands(uint64_t CatNB1_bands)
{
  PrintINFO("Cat.NB1 BANDS config = 0x%llx", CatNB1_bands)

  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B1) != 0U)
  {
    PrintINFO("CatNB1_B1")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B2) != 0U)
  {
    PrintINFO("CatNB1_B2")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B3) != 0U)
  {
    PrintINFO("CatNB1_B3")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B4) != 0U)
  {
    PrintINFO("CatNB1_B4")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B5) != 0U)
  {
    PrintINFO("CatNB1_B5")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B8) != 0U)
  {
    PrintINFO("CatNB1_B8")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B12) != 0U)
  {
    PrintINFO("CatNB1_B12")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B13) != 0U)
  {
    PrintINFO("CatNB1_B13")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B18) != 0U)
  {
    PrintINFO("CatNB1_B18")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B19) != 0U)
  {
    PrintINFO("CatNB1_B19")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B20) != 0U)
  {
    PrintINFO("CatNB1_B20")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B26) != 0U)
  {
    PrintINFO("CatNB1_B26")
  }
  if ((CatNB1_bands & (unsigned long long)_QCFGbandCatNB1_B28) != 0U)
  {
    PrintINFO("CatNB1_B28")
  }
}

static void display_user_friendly_mode_and_bands_config(void)
{
  uint8_t catM1_on = 0U;
  uint8_t catNB1_on = 0U;
  uint8_t gsm_on = 0U;
  ATCustom_BG96_QCFGscanseq_t scanseq_1st, scanseq_2nd, scanseq_3rd;

#if 0 /* for DEBUG */
  /* display modem raw values */
  display_decoded_CatM1_bands(bg96_mode_and_bands_config.CatM1_bands);
  display_decoded_CatNB1_bands(bg96_mode_and_bands_config.CatNB1_bands);
  display_decoded_GSM_bands(bg96_mode_and_bands_config.gsm_bands);

  PrintINFO("nw_scanmode = 0x%x", bg96_mode_and_bands_config.nw_scanmode)
  PrintINFO("iot_op_mode = 0x%x", bg96_mode_and_bands_config.iot_op_mode)
  PrintINFO("nw_scanseq = 0x%x", bg96_mode_and_bands_config.nw_scanseq)
#endif

  PrintINFO(">>>>> BG96 mode and bands configuration <<<<<")
  /* LTE bands */
  if ((bg96_mode_and_bands_config.nw_scanmode == _QCFGscanmodeAuto) ||
      (bg96_mode_and_bands_config.nw_scanmode == _QCFGscanmodeLTEOnly))
  {
    if ((bg96_mode_and_bands_config.iot_op_mode == _QCFGiotopmodeCatM1CatNB1) ||
        (bg96_mode_and_bands_config.iot_op_mode == _QCFGiotopmodeCatM1))
    {
      /* LTE Cat.M1 active */
      catM1_on = 1U;
    }
    if ((bg96_mode_and_bands_config.iot_op_mode == _QCFGiotopmodeCatM1CatNB1) ||
        (bg96_mode_and_bands_config.iot_op_mode == _QCFGiotopmodeCatNB1))
    {
      /* LTE Cat.NB1 active */
      catNB1_on = 1U;
    }
  }

  /* GSM bands */
  if ((bg96_mode_and_bands_config.nw_scanmode == _QCFGscanmodeAuto) ||
      (bg96_mode_and_bands_config.nw_scanmode == _QCFGscanmodeGSMOnly))
  {
    /* GSM active */
    gsm_on = 1U;
  }

  /* Search active techno */
  scanseq_1st = ((ATCustom_BG96_QCFGscanseq_t)bg96_mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x00FF0000U) >> 16;
  scanseq_2nd = ((ATCustom_BG96_QCFGscanseq_t)bg96_mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x0000FF00U) >> 8;
  scanseq_3rd = ((ATCustom_BG96_QCFGscanseq_t)bg96_mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x000000FFU);
  PrintDBG("decoded scanseq: 0x%lx -> 0x%lx -> 0x%lx", scanseq_1st, scanseq_2nd, scanseq_3rd)

  uint8_t rank = 1U;
  if (1U == display_if_active_band(scanseq_1st, rank, catM1_on, catNB1_on, gsm_on))
  {
    rank++;
  }
  if (1U == display_if_active_band(scanseq_2nd, rank, catM1_on, catNB1_on, gsm_on))
  {
    rank++;
  }
  display_if_active_band(scanseq_3rd, rank, catM1_on, catNB1_on, gsm_on);

  PrintINFO(">>>>> ................................. <<<<<")
}

static uint8_t display_if_active_band(ATCustom_BG96_QCFGscanseq_t scanseq,
                                      uint8_t rank,
                                      uint8_t catM1_on,
                                      uint8_t catNB1_on,
                                      uint8_t gsm_on)
{
  uint8_t retval = 0U;

  if (scanseq == (ATCustom_BG96_QCFGscanseq_t)_QCFGscanseqGSM)
  {
    if (gsm_on == 1U)
    {
      PrintINFO("GSM band active (scan rank = %d)", rank)
      display_decoded_GSM_bands(bg96_mode_and_bands_config.gsm_bands);

      retval = 1U;
    }
  }
  else if (scanseq == (ATCustom_BG96_QCFGscanseq_t)_QCFGscanseqLTECatM1)
  {
    if (catM1_on == 1U)
    {
      PrintINFO("LTE Cat.M1 band active (scan rank = %d)", rank)
      display_decoded_CatM1_bands(bg96_mode_and_bands_config.CatM1_bands);
      retval = 1U;
    }
  }
  else if (scanseq == (ATCustom_BG96_QCFGscanseq_t)_QCFGscanseqLTECatNB1)
  {
    if (catNB1_on == 1U)
    {
      PrintINFO("LTE Cat.NB1 band active (scan rank = %d)", rank)
      display_decoded_CatNB1_bands(bg96_mode_and_bands_config.CatNB1_bands);
      retval = 1U;
    }
  }
  else
  {
    /* scanseq value not managed */
  }

  return (retval);
}

static void socketHeaderRX_reset(void)
{
  memset((void *)SocketHeaderDataRx_Buf, 0, 4U);
  SocketHeaderDataRx_Cpt = 0U;
}
static void SocketHeaderRX_addChar(char *rxchar)
{
  if (SocketHeaderDataRx_Cpt < 4U)
  {
    memcpy((void *)&SocketHeaderDataRx_Buf[SocketHeaderDataRx_Cpt], (void *)rxchar, sizeof(char));
    SocketHeaderDataRx_Cpt++;
  }
}
static uint16_t SocketHeaderRX_getSize(void)
{
  uint16_t retval = (uint16_t) ATutil_convertStringToInt((uint8_t *)SocketHeaderDataRx_Buf, (uint16_t)SocketHeaderDataRx_Cpt);
  return (retval);
}

static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt)
{
  /* clear all parameters exxcept the index:
   * save the index, reset the structure and recopy saved index
   */
  uint8_t saved_idx = p_modem_ctxt->persist.ping_resp_urc.index;
  memset((void *)&p_modem_ctxt->persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_modem_ctxt->persist.ping_resp_urc.index = saved_idx;
}
/* ###########################  END CUSTOMIZATION PART  ########################### */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

