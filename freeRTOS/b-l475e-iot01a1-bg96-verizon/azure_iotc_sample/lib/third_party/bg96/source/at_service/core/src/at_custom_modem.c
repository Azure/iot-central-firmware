/**
  ******************************************************************************
  * @file    at_custom_modem.c
  * @author  MCD Application Team
  * @brief   This file provides common code for the modems
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

/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "at_custom_modem.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define MATCHING_USER_AND_MODEM_CID (1U)  /* set to 1 to force user CID = modem CID */
#define MAX_CGEV_PARAM_SIZE         (32U)
#define ATC_GET_MINIMUM_SIZE(a,b) ((a)<(b)?(a):(b))

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_MODEM == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCModem:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCModem:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCModem API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCModem ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P1, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("ATCModem:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("ATCModem:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("ATCModem API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("ATCModem ERROR:" format "\n\r", ## args);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_MODEM */

/* TWO MACROS USED TO SIMPLIFY CODE WHEN MULTIPLE PARAMETERS EXPECTED FOR AN AT-COMMAND ANSWER */
#define START_PARAM_LOOP()  PrintDBG("rank = %d",element_infos->param_rank)\
                            uint8_t exitcode = 0U;\
                            at_endmsg_t msg_end = ATENDMSG_NO;\
                            do { if (msg_end == ATENDMSG_YES) {exitcode = 1U;}

#define END_PARAM_LOOP()    msg_end = atcc_extractElement(p_at_ctxt, p_msg_in, element_infos);\
                            if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
                          } while (exitcode == 0U);

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
/* LUT: correspondance between PDPTYPE enum and string (for CGDCONT) */
const atcm_pdp_type_LUT_t ACTM_PDP_TYPE_LUT[] =
{
  {CS_PDPTYPE_IP,     "IP"},
  {CS_PDPTYPE_IPV6,   "IPV6"},
  {CS_PDPTYPE_IPV4V6, "IPV4V6"},
  {CS_PDPTYPE_PPP,    "PPP"},
};

/* Private function prototypes -----------------------------------------------*/
static void reserve_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt,
                              CS_PDN_conf_id_t conf_id,
                              uint8_t reserved_modem_cid);
static void affect_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt,
                             CS_PDN_conf_id_t conf_id);
static void reset_pdn_event(atcustom_persistent_context_t *p_persistent_ctxt);
static void display_clear_network_state(CS_NetworkRegState_t state);
static CS_NetworkRegState_t convert_NetworkState(uint32_t state);
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt,
                                                            csint_ip_addr_info_t *ip_addr_struct);
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt,
                                        atcustom_modem_context_t *p_modem_ctxt,
                                        IPC_RxMessage_t *p_msg_in,
                                        at_element_info_t *element_infos);
static void set_error_report(csint_error_type_t err_type, atcustom_modem_context_t *p_modem_ctxt);

/* Private function Definition -----------------------------------------------*/
/*
*  Reserve a modem modem cid (normally used only for PDN_PREDEF_CONFIG at startup time)
*/
static void reserve_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id, uint8_t reserved_modem_cid)
{
#if (MATCHING_USER_AND_MODEM_CID == 0U)
  /* User PDP CID and Modem CID are independant */
  for (uint8_t i = 0; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (reserved_modem_cid == p_persistent_ctxt->modem_cid_table[i].cid_value)
    {
      /* reserve this modem cid */
      p_persistent_ctxt->modem_cid_table[i].pdn_defined = AT_TRUE;
      p_persistent_ctxt->modem_cid_table[i].affected_config = conf_id;
      return;
    }
  }
#else
  /* User PDP CID and Modem CID are matching */
  /*UNUSED(conf_id);*/

  /* valid only for CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5 */
  if ((reserved_modem_cid == CS_PDN_USER_CONFIG_1) ||
      (reserved_modem_cid == CS_PDN_USER_CONFIG_2) ||
      (reserved_modem_cid == CS_PDN_USER_CONFIG_3) ||
      (reserved_modem_cid == CS_PDN_USER_CONFIG_4) ||
      (reserved_modem_cid == CS_PDN_USER_CONFIG_5))
  {
    /* reserve this modem cid */
    for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
    {
      if (reserved_modem_cid == p_persistent_ctxt->modem_cid_table[i].cid_value)
      {
        /* reserve this modem cid */
        p_persistent_ctxt->modem_cid_table[i].pdn_defined = AT_TRUE;
        return;
      }
    }
  }
  else
  {
    PrintErr("Trying to affect a non-valid modem CID")
  }
#endif /* MATCHING_USER_AND_MODEM_CID */

  PrintErr("unexpected error reserve_modem_cid()")
  return;
}

/*
*  Affect a modem cid to the specified user PDP config
*/
static void affect_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id)
{
#if (MATCHING_USER_AND_MODEM_CID == 0U)
  for (uint8_t i = 0; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (p_persistent_ctxt->modem_cid_table[i].pdn_defined == AT_FALSE)
    {
      /* affect this modem cid */
      p_persistent_ctxt->modem_cid_table[i].pdn_defined = AT_TRUE;
      p_persistent_ctxt->modem_cid_table[i].affected_config = conf_id;
      PrintINFO("PDP cid (%d) affected to modem cid (%d)", conf_id, p_persistent_ctxt->modem_cid_table[i].cid_value)
      return;
    }
  }
#else
  /* User PDP CID and Modem CID are matching */

  /* only for CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5 */
  if ((conf_id == CS_PDN_USER_CONFIG_1) ||
      (conf_id == CS_PDN_USER_CONFIG_2) ||
      (conf_id == CS_PDN_USER_CONFIG_3) ||
      (conf_id == CS_PDN_USER_CONFIG_4) ||
      (conf_id == CS_PDN_USER_CONFIG_5))
  {
    /* reserve this modem cid */
    for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
    {
      if (conf_id == p_persistent_ctxt->modem_cid_table[i].cid_value)
      {
        /* reserve this modem cid */
        p_persistent_ctxt->modem_cid_table[i].pdn_defined = AT_TRUE;
        return;
      }
    }
  }
  else
  {
    PrintErr("Trying to affect a non-valid modem CID")
  }
#endif /* MATCHING_USER_AND_MODEM_CID */

  PrintErr("unexpected error affect_modem_cid()")
  return;
}


static void reset_pdn_event(atcustom_persistent_context_t *p_persistent_ctxt)
{
  p_persistent_ctxt->pdn_event.event_origine = CGEV_EVENT_UNDEFINE;
  p_persistent_ctxt->pdn_event.event_scope   = CGEV_EVENT_SCOPE_GLOBAL;
  p_persistent_ctxt->pdn_event.event_type    = CGEV_EVENT_UNDEFINE;
  p_persistent_ctxt->pdn_event.conf_id       = CS_PDN_NOT_DEFINED;
}

static void display_clear_network_state(CS_NetworkRegState_t state)
{
  switch (state)
  {
    case CS_NRS_NOT_REGISTERED_NOT_SEARCHING:
      PrintINFO("NetworkState = NOT_REGISTERED_NOT_SEARCHING")
      break;
    case CS_NRS_REGISTERED_HOME_NETWORK:
      PrintINFO("NetworkState = REGISTERED_HOME_NETWORK")
      break;
    case CS_NRS_NOT_REGISTERED_SEARCHING:
      PrintINFO("NetworkState = NOT_REGISTERED_SEARCHING")
      break;
    case CS_NRS_REGISTRATION_DENIED:
      PrintINFO("NetworkState = REGISTRATION_DENIED")
      break;
    case CS_NRS_UNKNOWN:
      PrintINFO("NetworkState = UNKNOWN")
      break;
    case CS_NRS_REGISTERED_ROAMING:
      PrintINFO("NetworkState = REGISTERED_ROAMING")
      break;
    case CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK:
      PrintINFO("NetworkState = REGISTERED_SMS_ONLY_HOME_NETWORK")
      break;
    case CS_NRS_REGISTERED_SMS_ONLY_ROAMING:
      PrintINFO("NetworkState = REGISTERED_SMS_ONLY_ROAMING")
      break;
    case CS_NRS_EMERGENCY_ONLY:
      PrintINFO("NetworkState = EMERGENCY_ONLY")
      break;
    case CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK:
      PrintINFO("NetworkState = REGISTERED_CFSB_NP_HOME_NETWORK")
      break;
    case CS_NRS_REGISTERED_CFSB_NP_ROAMING:
      PrintINFO("NetworkState = REGISTERED_CFSB_NP_ROAMING")
      break;
    default:
      PrintINFO("unknown state value")
      break;
  }
}

static CS_NetworkRegState_t convert_NetworkState(uint32_t state)
{
  CS_NetworkRegState_t retval = CS_NRS_UNKNOWN;

  switch (state)
  {
    case 0:
      retval = CS_NRS_NOT_REGISTERED_NOT_SEARCHING;
      break;
    case 1:
      retval = CS_NRS_REGISTERED_HOME_NETWORK;
      break;
    case 2:
      retval = CS_NRS_NOT_REGISTERED_SEARCHING;
      break;
    case 3:
      retval = CS_NRS_REGISTRATION_DENIED;
      break;
    case 4:
      retval = CS_NRS_UNKNOWN;
      break;
    case 5:
      retval = CS_NRS_REGISTERED_ROAMING;
      break;
    case 6:
      retval = CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK;
      break;
    case 7:
      retval = CS_NRS_REGISTERED_SMS_ONLY_ROAMING;
      break;
    case 8:
      retval = CS_NRS_EMERGENCY_ONLY;
      break;
    case 9:
      retval = CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK;
      break;
    case 10:
      retval = CS_NRS_REGISTERED_CFSB_NP_ROAMING;
      break;
    default:
      retval = CS_NRS_UNKNOWN;
      break;
  }

  display_clear_network_state(retval);

  return (retval);
}

/*
 * Try to find user cid with matching IP address
 */
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt, csint_ip_addr_info_t *ip_addr_struct)
{
  CS_PDN_conf_id_t user_cid = CS_PDN_NOT_DEFINED;

  /* seach user config ID corresponding to this IP address */
  for (uint8_t loop = 0U; loop < MODEM_MAX_NB_PDP_CTXT; loop++)
  {
    PrintDBG("[Compare ip addr with user cid=%d]: <%s> vs <%s>",
             loop,
             (char *)&ip_addr_struct->ip_addr_value,
             (char *)&p_persistent_ctxt->modem_cid_table[loop].ip_addr_infos.ip_addr_value)

    /* quick and dirty solution
     * should implement a better solution */
    uint8_t size1, size2, minsize;
    size1 = (uint8_t) strlen((char *)&ip_addr_struct->ip_addr_value);
    size2 = (uint8_t) strlen((char *)&p_persistent_ctxt->modem_cid_table[loop].ip_addr_infos.ip_addr_value);
    minsize = (size1 < size2) ? size1 : size2;
    if ((0 == memcmp((AT_CHAR_t *)&ip_addr_struct->ip_addr_value[0],
                     (AT_CHAR_t *)&p_persistent_ctxt->modem_cid_table[loop].ip_addr_infos.ip_addr_value[0],
                     (size_t) minsize)) &&
        (minsize != 0U)
       )
    {
      user_cid = (CS_PDN_conf_id_t)loop;
      PrintDBG("Found matching user cid=%d", user_cid)
    }
  }

  return (user_cid);
}

/*
 * Analyze +CME ERROR
*/
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                        IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter analyze_CmeError_CPIN()")

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PrintDBG("CME ERROR parameter received:")
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
    }

    /* convert string to test to upper case */
    ATutil_convertStringToUpperCase(&line[0], 32U);

    /* extract value and compare it to expected value */
    if ((char *) strstr((const char *)&line[0], "SIM NOT INSERTED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_NOT_INSERTED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PIN NECESSARY") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PIN REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PUK REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM FAILURE") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_FAILURE;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM BUSY") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_BUSY;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM WRONG") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_WRONG;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "INCORRECT PASSWORD") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_INCORRECT_PASSWORD;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PIN2 REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PUK2 REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
  }
  END_PARAM_LOOP()

  return (retval);
}

/*
 * Update error report that will be sent to Cellular Service
 */
static void set_error_report(csint_error_type_t err_type, atcustom_modem_context_t *p_modem_ctxt)
{
  p_modem_ctxt->SID_ctxt.error_report.error_type = err_type;

  switch (err_type)
  {
    case CSERR_SIM:
      p_modem_ctxt->SID_ctxt.error_report.sim_state = p_modem_ctxt->persist.sim_state;
      break;

    default:
      /* nothing to do*/
      break;
  }
}

/* Functions Definition ------------------------------------------------------*/
/*
*  Put IP Address infos for the selected PDP config
*/
void atcm_put_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt,
                               uint8_t modem_cid,
                               csint_ip_addr_info_t *ip_addr_info)
{
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (p_persistent_ctxt->modem_cid_table[i].cid_value == modem_cid)
    {
      /* save IP address parameters */
      memcpy((void *)& p_persistent_ctxt->modem_cid_table[i].ip_addr_infos,
             (void *)ip_addr_info,
             sizeof(csint_ip_addr_info_t));
      return;
    }
  }

  PrintErr("unexpected error put_IP_address_infos()")
  return;
}


/*
*  Get IP Address infos for the selected PDP config
*/
void atcm_get_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt,
                               CS_PDN_conf_id_t conf_id,
                               csint_ip_addr_info_t  *ip_addr_info)
{
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (p_persistent_ctxt->modem_cid_table[i].affected_config == conf_id)
    {
      /* retrieve IP address parameters */
      memcpy((void *)ip_addr_info,
             (void *)& p_persistent_ctxt->modem_cid_table[i].ip_addr_infos,
             sizeof(csint_ip_addr_info_t));
      return;
    }
  }

  PrintErr("unexpected error get_IP_address_infos()")
  return;
}

/*
*  Get conf_ig in current SID context (interpret default if needed)
*/
CS_PDN_conf_id_t atcm_get_cid_current_SID(atcustom_modem_context_t *p_modem_ctxt)
{
  /* get conf_id received for current SID */
  CS_PDN_conf_id_t current_conf_id = p_modem_ctxt->SID_ctxt.pdn_conf_id;

  /* if default PDN is required, retrieve corresponding conf_id */
  if (current_conf_id == CS_PDN_CONFIG_DEFAULT)
  {
    current_conf_id = p_modem_ctxt->persist.pdn_default_conf_id;
    PrintDBG("Default PDP context selected (conf_id = %d)", current_conf_id)
  }

  return (current_conf_id);
}

/**
  * @brief  This function affects a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom)
  */
at_status_t atcm_socket_reserve_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PrintINFO("socket handle %d not valid", sockHandle)
    return (ATSTATUS_ERROR);
  }

  p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
  p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
  p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;

  return (retval);
}

/**
  * @brief  This function release a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom)
  */
at_status_t atcm_socket_release_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PrintINFO("socket handle %d not valid", sockHandle)
    return (ATSTATUS_ERROR);
  }

  if ((p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc == AT_TRUE) ||
      (p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc == AT_TRUE))
  {
    PrintINFO("Warning, there was pending URC for socket handle %d: (%d)data pending urc,(%d) closed by remote urc",
              sockHandle,
              p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc,
              p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc)
  }

  /* p_modem_ctxt->persist.socket[sockHandle].socket_connId_free = AT_TRUE; */
  p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
  p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
  p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;

  return (retval);
}

/**
  * @brief  This function returns the modem connection Id (ID shared between at-custom and the modem)
  *         corresponding to a socket handle (ID shared between upper layers and at-custom)
  */
uint32_t atcm_socket_get_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  uint32_t cid;

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PrintINFO("socket handle %d not valid", sockHandle)
    return (0U);
  }

  /* find  connectid correspondig to this socket_handle */
  cid = (uint32_t)(p_modem_ctxt->persist.socket[sockHandle].socket_connId_value);

  return (cid);
}

/**
  * @brief  This function returns the socket handle (ID shared between upper layers and at-custom)
  *         corresponding to modem connection Id (ID shared between at-custom and the modem)
  */
socket_handle_t atcm_socket_get_socket_handle(atcustom_modem_context_t *p_modem_ctxt, uint32_t modemCID)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    if (p_modem_ctxt->persist.socket[i].socket_connId_value == modemCID)
    {
      sockHandle = (socket_handle_t)i;
    }
  }

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PrintINFO("Can not find valid socket handle for modem CID=%ld", modemCID)
  }

  return (sockHandle);
}

/**
  * @brief  This function set the "socket data received" URC for a
  *         socket handle (ID shared between upper layers and at-custom)
  */
at_status_t atcm_socket_set_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PrintAPI("enter atcm_socket_set_urc_data_pending sockHandle=%d", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_TRUE;
    p_modem_ctxt->persist.urc_avail_socket_data_pending = AT_TRUE;
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  This function set the "socket closed by remote" URC for a
  *         socket handle (ID shared between upper layers and at-custom)
  */
at_status_t atcm_socket_set_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PrintAPI("enter atcm_socket_set_urc_closed_by_remote sockHandle=%d", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_TRUE;
    p_modem_ctxt->persist.urc_avail_socket_closed_by_remote = AT_TRUE;
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  This function returns the socket handle of "socket data received" URC
  *         and clears it
  */
socket_handle_t atcm_socket_get_hdle_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  PrintAPI("enter atcm_socket_get_hdle_urc_data_pending")

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    if (p_modem_ctxt->persist.socket[i].socket_data_pending_urc == AT_TRUE)
    {
      sockHandle = (socket_handle_t)i;
      /* clear this URC */
      p_modem_ctxt->persist.socket[i].socket_data_pending_urc = AT_FALSE;
      break;
    }
  }

  return (sockHandle);
}

/**
  * @brief  This function returns the socket handle of "socket closed by remote" URC
  *         and clears it
  */
socket_handle_t atcm_socket_get_hdlr_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  PrintAPI("enter atcm_socket_get_hdlr_urc_closed_by_remote")

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    if (p_modem_ctxt->persist.socket[i].socket_closed_pending_urc == AT_TRUE)
    {
      sockHandle = (socket_handle_t)i;
      /* clear this URC */
      p_modem_ctxt->persist.socket[i].socket_closed_pending_urc = AT_FALSE;
      break;
    }
  }

  return (sockHandle);
}

/**
  * @brief  This function returns if there are pending "socket data received" URC
  */
at_bool_t atcm_socket_remaining_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter atcm_socket_remaining_urc_data_pending")
  at_bool_t remain = AT_FALSE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    if (p_modem_ctxt->persist.socket[i].socket_data_pending_urc == AT_TRUE)
    {
      /* at least one remaining URC */
      remain = AT_TRUE;
      break;
    }
  }

  return (remain);
}

at_bool_t atcm_socket_is_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_bool_t retval = AT_FALSE;

  PrintAPI("enter atcm_socket_is_connected sockHandle=%d", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    if (p_modem_ctxt->persist.socket[sockHandle].socket_connected == AT_TRUE)
    {
      /* socket is currently connected */
      retval = AT_TRUE;
    }
  }

  return (retval);
}

at_status_t atcm_socket_set_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PrintAPI("enter atcm_socket_set_connected sockHandle=%d", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_TRUE;
  }

  return (retval);
}

/**
  * @brief  This function returns if there are pending "socket closed by remote" URC
  */
at_bool_t atcm_socket_remaining_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter atcm_socket_remaining_urc_closed_by_remote")

  at_bool_t remain = AT_FALSE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    if (p_modem_ctxt->persist.socket[i].socket_closed_pending_urc == AT_TRUE)
    {
      /* at least one remaining URC */
      remain = AT_TRUE;
      break;
    }
  }

  return (remain);
}

/* functions ------------------------------------------------------------------ */

/**
  * @brief  Search command string corresponding to a command Id
  *
  * @param  p_modem_ctxt modem context
  * @param  cmd_id Id of the command to find
  * @retval string of the command name
  */
const AT_CHAR_t *atcm_get_CmdStr(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  uint16_t i;

  /* check if this is the invalid cmd id*/
  if (cmd_id != _AT_INVALID)
  {
    /* search in LUT the cmd ID */
    for (i = 0U; i < p_modem_ctxt->modem_LUT_size; i++)
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        return (const AT_CHAR_t *)(&p_modem_ctxt->p_modem_LUT[i].cmd_str);
      }
    }
  }

  /* return default value */
  PrintDBG("exit atcm_get_CmdStr() with default value ")
  return (((uint8_t *)""));
}

/**
  * @brief  Search timeout corresponding to a command Id
  *
  * @param  p_modem_ctxt modem context
  * @param  cmd_id Id of the command to find
  * @retval timeout for this command
  */
uint32_t atcm_get_CmdTimeout(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  uint16_t i;

  /* check if this is the invalid cmd id */
  if (cmd_id != _AT_INVALID)
  {
    /* search in LUT the cmd ID */
    for (i = 0U; i < p_modem_ctxt->modem_LUT_size; i++)
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        return (p_modem_ctxt->p_modem_LUT[i].cmd_timeout);
      }
    }
  }

  /* return default value */
  PrintDBG("exit atcm_get_CmdTimeout() with default value ")
  return (MODEM_DEFAULT_TIMEOUT);
}

/**
  * @brief  Search Build Function corresponding to a command Id
  *
  * @param  p_modem_ctxt modem context
  * @param  cmd_id Id of the command to find
  * @retval BuildFunction
  */
CmdBuildFuncTypeDef atcm_get_CmdBuildFunc(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  uint16_t i;

  if (cmd_id != _AT_INVALID)
  {
    /* search in LUT the cmd ID */
    for (i = 0U; i < p_modem_ctxt->modem_LUT_size; i++)
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        return (p_modem_ctxt->p_modem_LUT[i].cmd_BuildFunc);
      }
    }
  }

  /* return default value */
  PrintDBG("exit atcm_get_CmdBuildFunc() with default value ")
  return (fCmdBuild_NoParams);
}

/**
  * @brief  Search Analyze Function corresponding to a command Id
  *
  * @param  p_modem_ctxt modem context
  * @param  cmd_id Id of the command to find
  * @retval AnalyzeFunction
  */
CmdAnalyzeFuncTypeDef atcm_get_CmdAnalyzeFunc(atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  uint16_t i;

  if (cmd_id != _AT_INVALID)
  {
    /* search in LUT the cmd ID */
    for (i = 0U; i < p_modem_ctxt->modem_LUT_size; i++)
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        return (p_modem_ctxt->p_modem_LUT[i].rsp_AnalyzeFunc);
      }
    }
  }

  /* return default value */
  PrintDBG("exit atcm_get_CmdAnalyzeFunc() with default value ")
  return (fRspAnalyze_None);
}

const AT_CHAR_t *atcm_get_PDPtypeStr(CS_PDPtype_t pdp_type)
{
  uint16_t i;
  uint16_t max_array_size = (sizeof(ACTM_PDP_TYPE_LUT) / sizeof(atcm_pdp_type_LUT_t));

  for (i = 0U; i < max_array_size; i++)
  {
    if (pdp_type == ACTM_PDP_TYPE_LUT[i].pdp_type)
    {
      return ((const AT_CHAR_t *)(&ACTM_PDP_TYPE_LUT[i].pdp_type_string));
    }
  }

  /* string no found, return empty string */
  return (((uint8_t *)""));
}

/* --------------------------------------------------------------------------------------------------------- */

/*
*  Program an AT command: answer is mandatory, an error will be raised if no answer received before timeout
*/
void atcm_program_AT_CMD(atcustom_modem_context_t *p_modem_ctxt,
                         atparser_context_t *p_atp_ctxt,
                         at_type_t cmd_type,
                         uint32_t cmd_id,
                         atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = cmd_type;
  /* command id */
  p_atp_ctxt->current_atcmd.id = cmd_id;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* an answer is expected */
  p_atp_ctxt->answer_expected = CMD_MANDATORY_ANSWER_EXPECTED;

  /* set command timeout according to LUT */
  p_atp_ctxt->cmd_timeout =  atcm_get_CmdTimeout(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
}

/*
*  Program an AT command: answer is optional, no error will be raised if no answer received before timeout
*/
void atcm_program_AT_CMD_ANSWER_OPTIONAL(atcustom_modem_context_t *p_modem_ctxt,
                                         atparser_context_t *p_atp_ctxt,
                                         at_type_t cmd_type,
                                         uint32_t cmd_id,
                                         atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = cmd_type;
  /* command id */
  p_atp_ctxt->current_atcmd.id = cmd_id;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* an answer is expected */
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;

  /* set command timeout according to LUT */
  p_atp_ctxt->cmd_timeout =  atcm_get_CmdTimeout(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
}

/*
*  Program an event to wait: if event does not occur before the specified time, an error will be raised
*/
void atcm_program_WAIT_EVENT(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final)
{
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  p_atp_ctxt->current_atcmd.id = _AT_INVALID;
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  p_atp_ctxt->answer_expected = CMD_MANDATORY_ANSWER_EXPECTED;
  p_atp_ctxt->cmd_timeout = tempo_value;
}

/*
*  Program a tempo: nothing special expected, if no event there is no error raised
*/
void atcm_program_TEMPO(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final)
{
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  p_atp_ctxt->current_atcmd.id = _AT_INVALID;
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  p_atp_ctxt->cmd_timeout = tempo_value;
}

/*
*  No command to send: nothing to send or to wait, last command
*/
void atcm_program_NO_MORE_CMD(atparser_context_t *p_atp_ctxt)
{
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  p_atp_ctxt->current_atcmd.id = _AT_INVALID;
  p_atp_ctxt->is_final_cmd = 1U;
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  p_atp_ctxt->cmd_timeout = 0U;
}

/*
*  Skip command to send: nothing to send or to wait, not the last command
*/
void atcm_program_SKIP_CMD(atparser_context_t *p_atp_ctxt)
{
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  p_atp_ctxt->current_atcmd.id = _AT_INVALID;
  p_atp_ctxt->is_final_cmd = 0U;
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  p_atp_ctxt->cmd_timeout = 0U;
}

/* --------------------------------------------------------------------------------------------------------- */
void atcm_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter atcm_modem_init")

  /* reset all contexts at init */
  atcm_reset_persistent_context(&p_modem_ctxt->persist);
  atcm_reset_SID_context(&p_modem_ctxt->SID_ctxt);
  atcm_reset_CMD_context(&p_modem_ctxt->CMD_ctxt);
  atcm_reset_SOCKET_context(p_modem_ctxt);
  p_modem_ctxt->state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

void atcm_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter atcm_modem_reset")

  /* reset all contexts except SID when reset */
  atcm_reset_persistent_context(&p_modem_ctxt->persist);
  atcm_reset_CMD_context(&p_modem_ctxt->CMD_ctxt);
  atcm_reset_SOCKET_context(p_modem_ctxt);
  p_modem_ctxt->state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

uint8_t atcm_modem_Standard_checkEndOfMsgCallback(atcustom_modem_context_t *p_modem_ctxt, uint8_t rxChar)
{
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(rxChar);*/
  uint8_t last_char = 0U;

  /* DO NOT USE THIS FUNCTION
  *  need a modem specific implementation (see an existing one to have an example)
  */

  return (last_char);
}

at_status_t atcm_modem_build_cmd(atcustom_modem_context_t *p_modem_ctxt,
                                 atparser_context_t *p_atp_ctxt,
                                 uint32_t *p_ATcmdTimeout)
{
  at_status_t retval = ATSTATUS_OK;

  /* 1- set the commande name (get it from LUT) */
  sprintf((char *)p_atp_ctxt->current_atcmd.name,
          (const char *)atcm_get_CmdStr(p_modem_ctxt,
                                        p_atp_ctxt->current_atcmd.id));

  PrintDBG("<modem custom> build the cmd %s (type=%d)",
           p_atp_ctxt->current_atcmd.name,
           p_atp_ctxt->current_atcmd.type)

  /* 2- set the command parameters (only for write or execution commands or for data) */
  if ((p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD) ||
      (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD) ||
      (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD))
  {
    retval = (atcm_get_CmdBuildFunc(p_modem_ctxt, p_atp_ctxt->current_atcmd.id))(p_atp_ctxt, p_modem_ctxt);
  }

  /* 3- set command timeout (has been sset in command programmation) */
  *p_ATcmdTimeout = p_atp_ctxt->cmd_timeout;
  PrintDBG("==== CMD TIMEOUT = %ld ====", *p_ATcmdTimeout)

  /* increment step in SID treatment */
  p_atp_ctxt->step++;

  PrintDBG("atcm_modem_build_cmd returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Prepare response buffer with SID infos. This rsp buffer is sent to Cellular Service.
  * @note   Only some SID return a response buffer.
  * @note   Called by AT-Core (via ATParser_get_rsp) when SID command returns OK.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  */
at_status_t atcm_modem_get_rsp(atcustom_modem_context_t *p_modem_ctxt,
                               atparser_context_t *p_atp_ctxt,
                               at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;

  /* prepare response for a SID
  *  all common behaviors for SID which are returning datas in rsp_buf have to be implemented here
  */

  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_GET_ATTACHSTATUS:
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf, CSMT_ATTACHSTATUS,
                               sizeof(CS_PSattach_t),
                               (void *)&p_modem_ctxt->SID_ctxt.attach_status) != DATAPACK_OK)
      {
        PrintErr("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_RECEIVE_DATA:
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf, CSMT_SOCKET_RXDATA,
                               sizeof(uint32_t),
                               (void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size) != DATAPACK_OK)
      {
        PrintErr("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_REGISTER_NET:
    case SID_CS_GET_NETSTATUS:
      /* Add EPS, GPRS and CS registration states (from CREG, CGREG, CEREG commands) */
      p_modem_ctxt->SID_ctxt.read_operator_infos.EPS_NetworkRegState = p_modem_ctxt->persist.eps_network_state;
      p_modem_ctxt->SID_ctxt.read_operator_infos.GPRS_NetworkRegState = p_modem_ctxt->persist.gprs_network_state;
      p_modem_ctxt->SID_ctxt.read_operator_infos.CS_NetworkRegState = p_modem_ctxt->persist.cs_network_state;
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf, CSMT_REGISTRATIONSTATUS,
                               sizeof(CS_RegistrationStatus_t),
                               (void *)&p_modem_ctxt->SID_ctxt.read_operator_infos) != DATAPACK_OK)
      {
        PrintErr("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_GET_IP_ADDRESS:
    {
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      csint_ip_addr_info_t ip_addr_info;
      memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));
      /* retrieve IP infos for request config_id */
      atcm_get_IP_address_infos(&p_modem_ctxt->persist, current_conf_id, &ip_addr_info);
      PrintDBG("retrieve IP address for cid %d = %s", current_conf_id, ip_addr_info.ip_addr_value)
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf, CSMT_GET_IP_ADDRESS,
                               sizeof(csint_ip_addr_info_t),
                               (void *)&ip_addr_info) != DATAPACK_OK)
      {
        PrintErr("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;
    }

    default:
      break;
  }

  return (retval);
}

/**
  * @brief  Prepare response buffer with URC infos. This rsp buffer is sent to Cellular Service.
  * @note   Called by AT-Core (via ATParser_get_urc) when URC are available.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  */
at_status_t atcm_modem_get_urc(atcustom_modem_context_t *p_modem_ctxt,
                               atparser_context_t *p_atp_ctxt,
                               at_buf_t *p_rsp_buf)
{
  /*UNUSED(p_atp_ctxt);*/

  at_status_t retval = ATSTATUS_OK;

  /* prepare response for an URC
  *  all common behaviors for URC have to be implemented here
  */

  /* URC for EPS network registration */
  if (p_modem_ctxt->persist.urc_avail_eps_network_registration == AT_TRUE)
  {
    PrintDBG("urc_avail_eps_network_registration")
    if (DATAPACK_writeStruct(p_rsp_buf,
                             CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS,
                             sizeof(CS_NetworkRegState_t),
                             (void *)&p_modem_ctxt->persist.eps_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_eps_network_registration = AT_FALSE;
  }
  /* URC for GPRS network registration */
  else if (p_modem_ctxt->persist.urc_avail_gprs_network_registration == AT_TRUE)
  {
    PrintDBG("urc_avail_gprs_network_registration")
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS, sizeof(CS_NetworkRegState_t), (void *)&p_modem_ctxt->persist.gprs_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_gprs_network_registration = AT_FALSE;
  }
  /* URC for CS network registration */
  else if (p_modem_ctxt->persist.urc_avail_cs_network_registration == AT_TRUE)
  {
    PrintDBG("urc_avail_cs_network_registration")
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_CS_NETWORK_REGISTRATION_STATUS, sizeof(CS_NetworkRegState_t), (void *)&p_modem_ctxt->persist.cs_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_cs_network_registration = AT_FALSE;
  }
  /* URC for EPS location info */
  else if ((p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE) || (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE))
  {
    PrintDBG("urc_avail_eps_location_info_tac or urc_avail_eps_location_info_ci")

    csint_location_info_t loc_struct = { .ci_updated = CELLULAR_FALSE, .lac_updated = CELLULAR_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.eps_location_info.lac;
      loc_struct.lac_updated = CELLULAR_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.eps_location_info.ci;
      loc_struct.ci_updated = CELLULAR_TRUE;
    }
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_EPS_LOCATION_INFO, sizeof(csint_location_info_t), (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_eps_location_info_tac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_eps_location_info_ci = AT_FALSE;
    }
  }
  /* URC for GPRS location info */
  else if ((p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE) || (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE))
  {
    PrintDBG("urc_avail_gprs_location_info_tac or urc_avail_gprs_location_info_ci")

    csint_location_info_t loc_struct = { .ci_updated = CELLULAR_FALSE, .lac_updated = CELLULAR_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.gprs_location_info.lac;
      loc_struct.lac_updated = CELLULAR_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.gprs_location_info.ci;
      loc_struct.ci_updated = CELLULAR_TRUE;
    }
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_GPRS_LOCATION_INFO, sizeof(csint_location_info_t), (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_gprs_location_info_lac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_gprs_location_info_ci = AT_FALSE;
    }
  }
  /* URC for CS location info */
  else if ((p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE) || (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE))
  {
    PrintDBG("urc_avail_cs_location_info_lac or urc_avail_cs_location_info_ci")

    csint_location_info_t loc_struct = { .ci_updated = CELLULAR_FALSE, .lac_updated = CELLULAR_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.cs_location_info.lac;
      loc_struct.lac_updated = CELLULAR_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.cs_location_info.ci;
      loc_struct.ci_updated = CELLULAR_TRUE;
    }
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_CS_LOCATION_INFO, sizeof(csint_location_info_t), (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags  (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_cs_location_info_lac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_cs_location_info_ci = AT_FALSE;
    }
  }
  /* URC for Signal Quality */
  else if (p_modem_ctxt->persist.urc_avail_signal_quality == AT_TRUE)
  {
    PrintDBG("urc_avail_signal_quality")

    CS_SignalQuality_t signal_quality_struct;
    signal_quality_struct.rssi = p_modem_ctxt->persist.signal_quality.rssi;
    signal_quality_struct.ber = p_modem_ctxt->persist.signal_quality.ber;
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_SIGNAL_QUALITY, sizeof(CS_SignalQuality_t), (void *)&signal_quality_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag  (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_signal_quality = AT_FALSE;
  }
  else if (p_modem_ctxt->persist.urc_avail_socket_data_pending == AT_TRUE)
  {
    PrintDBG("urc_avail_socket_data_pending")

    socket_handle_t sockHandle = atcm_socket_get_hdle_urc_data_pending(p_modem_ctxt);
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_SOCKET_DATA_PENDING, sizeof(socket_handle_t), (void *)&sockHandle) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag if no more socket data pending */
    p_modem_ctxt->persist.urc_avail_socket_data_pending = atcm_socket_remaining_urc_data_pending(p_modem_ctxt);
  }
  else if (p_modem_ctxt->persist.urc_avail_socket_closed_by_remote == AT_TRUE)
  {
    PrintDBG("urc_avail_socket_closed_by_remote")

    socket_handle_t sockHandle = atcm_socket_get_hdlr_urc_closed_by_remote(p_modem_ctxt);
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_SOCKET_CLOSED, sizeof(socket_handle_t), (void *)&sockHandle) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag if no more socket data pending */
    p_modem_ctxt->persist.urc_avail_socket_closed_by_remote = atcm_socket_remaining_urc_closed_by_remote(p_modem_ctxt);
  }
  else if (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE)
  {
    PrintDBG("urc_avail_pdn_event")

    /*  We are only interested by following +CGEV URC :
    *   +CGEV: NW DETACH : the network has forced a Packet domain detach (all contexts)
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>] : the nw has forced a contex deactivation
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] : context deactivated
    */
    if (p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW)
    {
      switch (p_modem_ctxt->persist.pdn_event.event_type)
      {
        case CGEV_EVENT_TYPE_DETACH:
          /*  we are in the case:
          *  +CGEV: NW DETACH
          */
          if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_PACKET_DOMAIN_EVENT, sizeof(csint_PDN_event_desc_t), (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
          {
            retval = ATSTATUS_ERROR;
          }
          break;

        case CGEV_EVENT_TYPE_DEACTIVATION:
          if (p_modem_ctxt->persist.pdn_event.event_scope == CGEV_EVENT_SCOPE_PDN)
          {
            /* we are in the case:
            *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
            */
            if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_PACKET_DOMAIN_EVENT, sizeof(csint_PDN_event_desc_t), (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
            {
              retval = ATSTATUS_ERROR;
            }
          }
          else
          {
            /* we are in the case:
            *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
            */
            if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_PACKET_DOMAIN_EVENT, sizeof(csint_PDN_event_desc_t), (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
            {
              retval = ATSTATUS_ERROR;
            }
          }
          break;

        default:
          PrintINFO("+CGEV URC discarded (NW), type=%d", p_modem_ctxt->persist.pdn_event.event_type)
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      PrintINFO("+CGEV URC discarded (ME)")
      retval = ATSTATUS_ERROR;
    }

    /* reset flag and pdn event (systematically to avoid never ending URC) */
    reset_pdn_event(&p_modem_ctxt->persist);
    p_modem_ctxt->persist.urc_avail_pdn_event = AT_FALSE;
  }
  /* PING response */
  else if (p_modem_ctxt->persist.urc_avail_ping_rsp == AT_TRUE)
  {
    PrintDBG("urc_avail_ping_rsp")
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_PING_RSP, sizeof(CS_Ping_response_t), (void *)&p_modem_ctxt->persist.ping_resp_urc) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_FALSE;
  }
  else if (p_modem_ctxt->persist.urc_avail_modem_events != CS_MDMEVENT_NONE)
  {
    PrintDBG("urc_avail_modem_events")
    if (DATAPACK_writeStruct(p_rsp_buf, CSMT_URC_MODEM_EVENT, sizeof(CS_ModemEvent_t), (void *)&p_modem_ctxt->persist.urc_avail_modem_events) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_modem_events = CS_MDMEVENT_NONE;
  }
  else
  {
    PrintErr("unexpected URC")
    retval = ATSTATUS_ERROR;
  }

  /* still some pending URC ? */
  if ((p_modem_ctxt->persist.urc_avail_eps_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_signal_quality == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_socket_data_pending == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_socket_closed_by_remote == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_ping_rsp == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_modem_events != CS_MDMEVENT_NONE))
  {
    retval = ATSTATUS_OK_PENDING_URC;
  }

  return (retval);
}

/**
  * @brief  Prepare response buffer with error infos. This rsp buffer is sent to Cellular Service.
  * @note   Called by AT-Core (via ATParser_get_error) when a command returns an error.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  */
at_status_t atcm_modem_get_error(atcustom_modem_context_t *p_modem_ctxt,
                                 atparser_context_t *p_atp_ctxt,
                                 at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;

  /* prepare error report */
  if (DATAPACK_writeStruct(p_rsp_buf, CSMT_ERROR_REPORT,
                           sizeof(csint_error_report_t),
                           (void *)&p_modem_ctxt->SID_ctxt.error_report) != DATAPACK_OK)
  {
    PrintErr("Buffer size problem")
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

at_status_t atcm_subscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  /* Retrieve urc event request: CEREG, CREG or CGREG ?
  *  note: only one event at same time
  */
  CS_UrcEvent_t urcEvent = p_modem_ctxt->SID_ctxt.urcEvent;

  /* is an event linked to CEREG ? */
  if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
  {
    /* if CEREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_eps_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_eps_locationInfo == CELLULAR_FALSE))
    {
      /* set event as subscribed */
      if (urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT)
      {
        p_modem_ctxt->persist.urc_subscript_eps_networkReg = CELLULAR_TRUE;
      }
      if (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO)
      {
        p_modem_ctxt->persist.urc_subscript_eps_locationInfo = CELLULAR_TRUE;
      }

      /* request all URC, we will filter them */
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CEREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CGREG ?  */
  else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
  {
    /* if CGREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_gprs_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_gprs_locationInfo == CELLULAR_FALSE))
    {
      /* set event as subscribed */
      if (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
      {
        p_modem_ctxt->persist.urc_subscript_gprs_networkReg = CELLULAR_TRUE;
      }
      if (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO)
      {
        p_modem_ctxt->persist.urc_subscript_gprs_locationInfo = CELLULAR_TRUE;
      }

      /* request all URC, we will filter them */
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CREG ? */
  else if ((urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
  {
    /* if CREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_cs_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_cs_locationInfo == CELLULAR_FALSE))
    {
      /* set event as subscribed */
      if (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT)
      {
        p_modem_ctxt->persist.urc_subscript_cs_networkReg = CELLULAR_TRUE;
      }
      if (urcEvent == CS_URCEVENT_CS_LOCATION_INFO)
      {
        p_modem_ctxt->persist.urc_subscript_cs_locationInfo = CELLULAR_TRUE;
      }

      /* request all URC, we will filter them */
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* nothing to do */
  }

  return (ATSTATUS_OK);
}

at_status_t atcm_unsubscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  /* Retrieve urc event request: CEREG, CREG or CGREG ?
  *  note: only one event at same time
  */
  CS_UrcEvent_t urcEvent = p_modem_ctxt->SID_ctxt.urcEvent;

  /* is an event linked to CEREG ? */
  if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_eps_networkReg = CELLULAR_FALSE;
    }
    if (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_eps_locationInfo = CELLULAR_FALSE;
    }

    /* if no more event for CEREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_eps_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_eps_locationInfo == CELLULAR_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CEREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CGREG ? */
  else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_networkReg = CELLULAR_FALSE;
    }
    if (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_locationInfo = CELLULAR_FALSE;
    }

    /* if no more event for CGREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_gprs_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_gprs_locationInfo == CELLULAR_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CGREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CREG ? */
  else if ((urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_cs_networkReg = CELLULAR_FALSE;
    }
    if (urcEvent == CS_URCEVENT_CS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_cs_locationInfo = CELLULAR_FALSE;
    }

    /* if no more event for CREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_cs_networkReg == CELLULAR_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_cs_locationInfo == CELLULAR_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, _AT_CREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* nothing to do */
  }

  return (ATSTATUS_OK);
}

void atcm_validate_ping_request(atcustom_modem_context_t *p_modem_ctxt)
{
  /* PING request is valid */

  /* copy SID ping parameters to persistent context */
  memcpy((void *)&p_modem_ctxt->persist.ping_infos, (void *)&p_modem_ctxt->SID_ctxt.ping_infos, sizeof(csint_ping_params_t));
  /* reset other parameters */
  memset((void *)&p_modem_ctxt->persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_modem_ctxt->persist.urc_avail_ping_rsp = AT_FALSE;
}

at_bool_t atcm_modem_event_received(atcustom_modem_context_t *p_modem_ctxt, CS_ModemEvent_t mdm_evt)
{
  at_bool_t event_subscribed = AT_FALSE;

  /* if the event received is subscribed, save it */
  if ((p_modem_ctxt->persist.modem_events_subscript & mdm_evt) != 0) /* bitmask check */
  {
    p_modem_ctxt->persist.urc_avail_modem_events |= mdm_evt;
    event_subscribed = AT_TRUE;
  }

  /* returns true only if event has been subscribed */
  return (event_subscribed);
}

void atcm_reset_persistent_context(atcustom_persistent_context_t *p_persistent_ctxt)
{
  PrintAPI("enter reset_persistent_context()")

  /* Modem in COMMAND mode by default */
  p_persistent_ctxt->modem_in_data_mode = AT_FALSE;

  /* URC subscriptions */
  p_persistent_ctxt->urc_subscript_eps_networkReg = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_eps_locationInfo = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_gprs_networkReg = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_gprs_locationInfo = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_cs_networkReg = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_cs_locationInfo = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_signalQuality = CELLULAR_FALSE;
  p_persistent_ctxt->urc_subscript_pdn_event = CELLULAR_FALSE;

  /* URC availabilities */
  p_persistent_ctxt->urc_avail_eps_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_eps_location_info_tac = AT_FALSE;
  p_persistent_ctxt->urc_avail_eps_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_location_info_lac = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_location_info_lac = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_signal_quality = AT_FALSE;
  p_persistent_ctxt->urc_avail_socket_data_pending = AT_FALSE;
  p_persistent_ctxt->urc_avail_socket_closed_by_remote = AT_FALSE;
  p_persistent_ctxt->urc_avail_pdn_event = AT_FALSE;

  /* Modem events subscriptions */
  p_persistent_ctxt->modem_events_subscript = CS_MDMEVENT_NONE;

  /* Modem events availabilities */
  p_persistent_ctxt->urc_avail_modem_events = CS_MDMEVENT_NONE;

  /* reset PDN event parameters */
  reset_pdn_event(p_persistent_ctxt);

  /* initialize allocation table of modem cid */
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    p_persistent_ctxt->modem_cid_table[i].cid_value = i + 1U; /* modem cid value starting at 1 (0 is reserved) */
    p_persistent_ctxt->modem_cid_table[i].pdn_defined = AT_FALSE;
    p_persistent_ctxt->modem_cid_table[i].affected_config = CS_PDN_NOT_DEFINED; /* not affected for the moment */
    p_persistent_ctxt->modem_cid_table[i].ip_addr_infos.ip_addr_type = CS_IPAT_INVALID;
    memset((void *)&p_persistent_ctxt->modem_cid_table[i].ip_addr_infos.ip_addr_value, 0, MAX_IP_ADDR_SIZE);
  }

#if (MATCHING_USER_AND_MODEM_CID == 1U)
  p_persistent_ctxt->modem_cid_table[0].cid_value = CS_PDN_PREDEF_CONFIG;
  p_persistent_ctxt->modem_cid_table[0].affected_config = CS_PDN_PREDEF_CONFIG;
  p_persistent_ctxt->modem_cid_table[1].cid_value = CS_PDN_USER_CONFIG_1;
  p_persistent_ctxt->modem_cid_table[1].affected_config = CS_PDN_USER_CONFIG_1;
  p_persistent_ctxt->modem_cid_table[2].cid_value = CS_PDN_USER_CONFIG_2;
  p_persistent_ctxt->modem_cid_table[2].affected_config = CS_PDN_USER_CONFIG_2;
  p_persistent_ctxt->modem_cid_table[3].cid_value = CS_PDN_USER_CONFIG_3;
  p_persistent_ctxt->modem_cid_table[3].affected_config = CS_PDN_USER_CONFIG_3;
  p_persistent_ctxt->modem_cid_table[4].cid_value = CS_PDN_USER_CONFIG_4;
  p_persistent_ctxt->modem_cid_table[4].affected_config = CS_PDN_USER_CONFIG_4;
  p_persistent_ctxt->modem_cid_table[5].cid_value = CS_PDN_USER_CONFIG_5;
  p_persistent_ctxt->modem_cid_table[5].affected_config = CS_PDN_USER_CONFIG_5;
#endif /* MATCHING_USER_AND_MODEM_CID */

  /* initialize PDP context parameters table */
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    p_persistent_ctxt->pdp_ctxt_infos[i].conf_id = CS_PDN_NOT_DEFINED; /* not used */
    memset((void *)&p_persistent_ctxt->pdp_ctxt_infos[i].apn, 0, MAX_APN_SIZE);
    memset((void *)&p_persistent_ctxt->pdp_ctxt_infos[i].pdn_conf, 0, sizeof(CS_PDN_configuration_t));
    /* set default PDP type = IPV4 */
    p_persistent_ctxt->pdp_ctxt_infos[i].pdn_conf.pdp_type = CS_PDPTYPE_IP;
  }

  /* set default PDP parameters (ie for PDN_PREDEF_CONFIG) from configuration file if defined */
  p_persistent_ctxt->pdn_default_conf_id = CS_PDN_PREDEF_CONFIG;
  p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].conf_id = CS_PDN_PREDEF_CONFIG;
#if defined(PDP_CONTEXT_DEFAULT_MODEM_CID)
  reserve_modem_cid(p_persistent_ctxt, CS_PDN_PREDEF_CONFIG, (uint8_t) PDP_CONTEXT_DEFAULT_MODEM_CID);
#endif /* PDP_CONTEXT_DEFAULT_MODEM_CID */
#if defined(PDP_CONTEXT_DEFAULT_TYPE)
  if (memcmp(PDP_CONTEXT_DEFAULT_TYPE, "IP", sizeof(PDP_CONTEXT_DEFAULT_TYPE)) == 0)
  {
    p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].pdn_conf.pdp_type = CS_PDPTYPE_IP;
  }
  else if (memcmp(PDP_CONTEXT_DEFAULT_TYPE, "IPV6", sizeof(PDP_CONTEXT_DEFAULT_TYPE)) == 0)
  {
    p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].pdn_conf.pdp_type = CS_PDPTYPE_IPV6;
  }
  else if (memcmp(PDP_CONTEXT_DEFAULT_TYPE, "IPV4V6", sizeof(PDP_CONTEXT_DEFAULT_TYPE)) == 0)
  {
    p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].pdn_conf.pdp_type = CS_PDPTYPE_IPV4V6;
  }
  else if (memcmp(PDP_CONTEXT_DEFAULT_TYPE, "PPP", sizeof(PDP_CONTEXT_DEFAULT_TYPE)) == 0)
  {
    p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].pdn_conf.pdp_type = CS_PDPTYPE_PPP;
  }
  else
  {
    PrintErr("Invalid PDP TYPE for default PDN")
  }
#endif /* PDP_CONTEXT_DEFAULT_TYPE */
#if defined(PDP_CONTEXT_DEFAULT_APN)
  if (sizeof(PDP_CONTEXT_DEFAULT_APN) <= MODEM_PDP_MAX_APN_SIZE)
  {
    memcpy((char *)&p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG].apn,
           &PDP_CONTEXT_DEFAULT_APN,
           sizeof(PDP_CONTEXT_DEFAULT_APN));
  }
#endif /* PDP_CONTEXT_DEFAULT_APN */

  /* socket */
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    p_persistent_ctxt->socket[i].socket_connId_value = ((uint8_t)i + 1U); /* socket ID range from 1 to 6, if a modem does not support this range => need to adapt */
    /* p_persistent_ctxt->socket[i].socket_connId_free = AT_TRUE; */
    p_persistent_ctxt->socket[i].socket_connected = AT_FALSE;
    p_persistent_ctxt->socket[i].socket_data_pending_urc = AT_FALSE;
    p_persistent_ctxt->socket[i].socket_closed_pending_urc = AT_FALSE;
  }

  /* other */
  p_persistent_ctxt->modem_at_ready = AT_FALSE;     /* modem ready to receive AT commands */
  p_persistent_ctxt->modem_sim_ready = AT_FALSE;    /* modem sim ready */
  p_persistent_ctxt->sim_pin_code_ready = AT_FALSE; /* modem pin code status */
  p_persistent_ctxt->cmee_level = CMEE_VERBOSE;
  p_persistent_ctxt->sim_state = CS_SIMSTATE_UNKNOWN;

  /* ping infos */
  memset((void *)&p_persistent_ctxt->ping_infos, 0, sizeof(csint_ping_params_t));
  memset((void *)&p_persistent_ctxt->ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_persistent_ctxt->urc_avail_ping_rsp = AT_FALSE;
}

void atcm_reset_SID_context(atcustom_SID_context_t *p_sid_ctxt)
{
  PrintAPI("enter reset_SID_context()")

  p_sid_ctxt->attach_status = CS_PS_DETACHED;

  memset((void *)&p_sid_ctxt->write_operator_infos, 0, sizeof(CS_OperatorSelector_t));
  p_sid_ctxt->write_operator_infos.format = CS_ONF_NOT_PRESENT;

  memset((void *)&p_sid_ctxt->read_operator_infos, 0, sizeof(CS_OperatorSelector_t));
  p_sid_ctxt->read_operator_infos.EPS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING;  /* default value */
  p_sid_ctxt->read_operator_infos.GPRS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING; /* default value */
  p_sid_ctxt->read_operator_infos.CS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING;   /* default value */
  p_sid_ctxt->read_operator_infos.optional_fields_presence = CS_RSF_NONE;

  p_sid_ctxt->modem_init.init = CS_CMI_MINI;
  p_sid_ctxt->modem_init.reset = CELLULAR_FALSE;
  memset((void *)&p_sid_ctxt->modem_init.pincode.pincode, '\0', sizeof(csint_pinCode_t));

  p_sid_ctxt->device_info = NULL;
  p_sid_ctxt->signal_quality = NULL;

  p_sid_ctxt->dns_request_infos = NULL;

  p_sid_ctxt->urcEvent = CS_URCEVENT_NONE;

  p_sid_ctxt->pdn_conf_id = CS_PDN_CONFIG_DEFAULT;

  memset((void *)&p_sid_ctxt->socketSendData_struct, 0, sizeof(csint_socket_data_buffer_t));

  memset((void *)&p_sid_ctxt->ping_infos, 0, sizeof(csint_ping_params_t));

  memset((void *)&p_sid_ctxt->error_report, 0, sizeof(csint_error_report_t));
  p_sid_ctxt->error_report.error_type = CSERR_UNKNOWN;
}

void atcm_reset_CMD_context(atcustom_CMD_context_t *p_cmd_ctxt)
{
  PrintAPI("enter reset_CMD_context()")

  p_cmd_ctxt->cgsn_write_cmd_param = CGSN_SN;
  p_cmd_ctxt->cgatt_write_cmd_param = CGATT_UNKNOWN;
  p_cmd_ctxt->cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
  p_cmd_ctxt->command_echo = AT_FALSE;
  p_cmd_ctxt->dce_full_resp_format = AT_TRUE;
  p_cmd_ctxt->pdn_state = AT_TRUE;
  p_cmd_ctxt->modem_cid = 0xFFFFU;
  p_cmd_ctxt->baud_rate = MODEM_UART_BAUDRATE;
  p_cmd_ctxt->cfun_value = 1U;
#if (CONFIG_MODEM_UART_RTS_CTS == 1)
  p_cmd_ctxt->flow_control_cts_rts = AT_TRUE;
#else
  p_cmd_ctxt->flow_control_cts_rts = AT_FALSE;
#endif /* CONFIG_MODEM_UART_RTS_CTS */
}

void atcm_reset_SOCKET_context(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter atcm_reset_SOCKET_context()")

  p_modem_ctxt->socket_ctxt.socket_info = NULL;
  memset((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata, 0, sizeof(csint_socket_data_buffer_t));
  p_modem_ctxt->socket_ctxt.socket_current_connId = 0U;
  p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size = 0U;
  p_modem_ctxt->socket_ctxt.socket_rx_count_bytes_received = 0U;

  p_modem_ctxt->socket_ctxt.socket_send_state = SocketSendState_No_Activity;
  p_modem_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_No_Activity;
  p_modem_ctxt->socket_ctxt.socket_RxData_state = SocketRxDataState_not_started;
}

at_status_t atcm_searchCmdInLUT(atcustom_modem_context_t *p_modem_ctxt,
                                atparser_context_t  *p_atp_ctxt,
                                IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  /*UNUSED(p_atp_ctxt);*/

  uint16_t i;

  element_infos->cmd_id_received = _AT_INVALID;

  /* check if we receive empty command */
  if (element_infos->str_size == 0U)
  {
    /* empty answer */
    element_infos->cmd_id_received = _AT;
    /* null size string */
    return (ATSTATUS_OK);
  }

  /* search in LUT the ID corresponding to command received */
  for (i = 0U; i < p_modem_ctxt->modem_LUT_size; i++)
  {
    /* if string length > 0 */
    if (strlen((const char *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str) > 0U)
    {
      /* compare strings size first */
      if ((strlen((const char *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str) == element_infos->str_size))
      {
        /* compare strings content */
        if (0 == memcmp((void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                        (const AT_CHAR_t *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str,
                        (size_t) element_infos->str_size))
        {
          PrintDBG("we received LUT#%ld : %s \r\n", (p_modem_ctxt->p_modem_LUT)[i].cmd_id, (p_modem_ctxt->p_modem_LUT)[i].cmd_str)

          element_infos->cmd_id_received = (p_modem_ctxt->p_modem_LUT)[i].cmd_id;
          return (ATSTATUS_OK);
        }
      }
    }
  }

  return (ATSTATUS_ERROR);
}

at_action_rsp_t atcm_check_text_line_cmd(atcustom_modem_context_t *p_modem_ctxt,
                                         at_context_t *p_at_ctxt,
                                         IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_ERROR;

  /* in this section, we treat all commands which can return text lines */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case _AT_CGMI:
      if (fRspAnalyze_CGMI(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_CGMM:
      if (fRspAnalyze_CGMM(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_CGMR:
      if (fRspAnalyze_CGMR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_CGSN:
      if (fRspAnalyze_CGSN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_GSN:
      if (fRspAnalyze_GSN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_IPR:
      if (fRspAnalyze_IPR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_CIMI:
      if (fRspAnalyze_CIMI(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case _AT_CGPADDR:
      if (fRspAnalyze_CGPADDR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    default:
      /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
      retval = ATACTION_RSP_NO_ACTION;
      break;
  }

  return (retval);
}

void atcm_set_modem_data_mode(atcustom_modem_context_t *p_modem_ctxt, at_bool_t val)
{
  /* set persistent context info about DATA or COMMAND mode */
  p_modem_ctxt->persist.modem_in_data_mode = val;
}

at_bool_t atcm_get_modem_data_mode(atcustom_modem_context_t *p_modem_ctxt)
{
  /* returns persistent context info about DATA or COMMAND mode */
  return (p_modem_ctxt->persist.modem_in_data_mode);
}

at_status_t atcm_retrieve_SID_parameters(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  at_status_t retval = ATSTATUS_OK;

  /* only retrieve SID parameters on first call (step = 0)*/
  if (p_atp_ctxt->step == 0U)
  {
    switch (p_atp_ctxt->current_SID)
    {
      case SID_CS_MODEM_CONFIG:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_MODEMCONFIG, sizeof(CS_ModemConfig_t), (void *)&p_modem_ctxt->SID_ctxt.modem_config) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_INIT_MODEM:
        /* retrieve  client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_INITMODEM, sizeof(csint_modemInit_t), (void *)&p_modem_ctxt->SID_ctxt.modem_init) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_DEVICE_INFO:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_DEVICE_INFO, (void *)&p_modem_ctxt->SID_ctxt.device_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_SIGNAL_QUALITY:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_SIGNAL_QUALITY, (void *)&p_modem_ctxt->SID_ctxt.signal_quality) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_REGISTER_NET:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_OPERATORSELECT, sizeof(CS_OperatorSelector_t), (void *)&p_modem_ctxt->SID_ctxt.write_operator_infos) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SUSBCRIBE_NET_EVENT:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_URC_EVENT, sizeof(CS_UrcEvent_t), (void *)&p_modem_ctxt->SID_ctxt.urcEvent) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_UNSUSBCRIBE_NET_EVENT:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_URC_EVENT, sizeof(CS_UrcEvent_t), (void *)&p_modem_ctxt->SID_ctxt.urcEvent) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DIAL_COMMAND:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_SOCKET_INFO, (void *)&p_modem_ctxt->socket_ctxt.socket_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SEND_DATA:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                CSMT_SOCKET_DATA_BUFFER,
                                sizeof(csint_socket_data_buffer_t),
                                &p_modem_ctxt->SID_ctxt.socketSendData_struct) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_RECEIVE_DATA:
        /* retrieve pointer on client structure */
        if (DATAPACK_readStruct(p_atp_ctxt->p_cmd_input, CSMT_SOCKET_DATA_BUFFER, sizeof(csint_socket_data_buffer_t), (void *)&p_modem_ctxt->socket_ctxt.socketReceivedata) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SOCKET_CLOSE:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_SOCKET_INFO, (void *)&p_modem_ctxt->socket_ctxt.socket_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_RESET:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_RESET, sizeof(CS_Reset_t), (void *)&p_modem_ctxt->SID_ctxt.reset_type) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_ACTIVATE_PDN:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_ACTIVATE_PDN, sizeof(CS_PDN_conf_id_t), (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DEACTIVATE_PDN:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_DEACTIVATE_PDN, sizeof(CS_PDN_conf_id_t), (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DEFINE_PDN:
      {
        csint_pdn_infos_t *ptr_pdn_infos;
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_DEFINE_PDN, (void *)&ptr_pdn_infos) == DATAPACK_OK)
        {
          /* recopy pdn configuration infos to persistant context */
          memcpy((void *)&p_modem_ctxt->persist.pdp_ctxt_infos[ptr_pdn_infos->conf_id],
                 (void *)ptr_pdn_infos,
                 sizeof(csint_pdn_infos_t));
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = ptr_pdn_infos->conf_id;
          /* affect a modem cid to this configuration */
          affect_modem_cid(&p_modem_ctxt->persist, ptr_pdn_infos->conf_id);
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
      }
      break;

      case SID_CS_SET_DEFAULT_PDN:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_SET_DEFAULT_PDN, sizeof(CS_PDN_conf_id_t), (void *)&p_modem_ctxt->persist.pdn_default_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_IP_ADDRESS:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_GET_IP_ADDRESS, sizeof(CS_PDN_conf_id_t), (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DNS_REQ:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_DNS_REQ, (void *)&p_modem_ctxt->SID_ctxt.dns_request_infos) == DATAPACK_OK)
        {
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = p_modem_ctxt->SID_ctxt.dns_request_infos->conf_id;
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SOCKET_CNX_STATUS:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input, CSMT_SOCKET_CNX_STATUS, (void *)&p_modem_ctxt->socket_ctxt.socket_cnx_infos) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SUSBCRIBE_MODEM_EVENT:
        /* retrieve client datas */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_MODEM_EVENT, sizeof(CS_ModemEvent_t), (void *)&p_modem_ctxt->persist.modem_events_subscript) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_PING_IP_ADDRESS:
        /* retrieve pointer on client structure */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input, CSMT_PING_ADDRESS, sizeof(csint_ping_params_t), (void *)&p_modem_ctxt->SID_ctxt.ping_infos) == DATAPACK_OK)
        {
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = p_modem_ctxt->SID_ctxt.ping_infos.conf_id;
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_CHECK_CNX:
      case SID_CS_POWER_ON:
      case SID_CS_POWER_OFF:
      case SID_CS_GET_NETSTATUS:
      case SID_CS_GET_ATTACHSTATUS:
      case SID_ATTACH_PS_DOMAIN:
      case SID_DETACH_PS_DOMAIN:
      case SID_CS_DATA_SUSPEND:
      case SID_CS_DATA_RESUME:
        PrintDBG("No data to unpack for SID %d", p_atp_ctxt->current_SID)
        retval = ATSTATUS_OK;
        break;

      case SID_CS_REGISTER_PDN_EVENT:
      case SID_CS_DEREGISTER_PDN_EVENT:
        PrintDBG("No data to unpack for SID %d", p_atp_ctxt->current_SID)
        retval = ATSTATUS_OK;
        break;

      /*********************
      LIST OF SID NOT IMPLEMENTED YET:
        SID_CS_AUTOACTIVATE_PDN,
        SID_CS_CONNECT,
        SID_CS_DISCONNECT,
        SID_CS_SOCKET_RECEIVE_DATA,
        SID_CS_RESET,

        SID_CS_DIAL_ONLINE, - NOT SUPPORTED
        SID_CS_SOCKET_CREATE, - not needed, config is done at CS level
        SID_CS_SOCKET_SET_OPTION, - not needed, config is done at CS level
        SID_CS_SOCKET_GET_OPTION, - not needed, config is done at CS level
      ***************/
      default:
        PrintErr("Missing treatment for SID %d", p_atp_ctxt->current_SID)
        retval = ATSTATUS_ERROR;
        break;
    }

  }

  return (retval);
}

/*
*  Get modem cid affected to requested PDP config
*/
uint8_t atcm_get_affected_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id)
{
  CS_PDN_conf_id_t current_conf_id = conf_id;

  /* if default PDN is required, retrieve corresponding conf_id */
  if (conf_id == CS_PDN_CONFIG_DEFAULT)
  {
    current_conf_id = p_persistent_ctxt->pdn_default_conf_id;
    PrintDBG("Default PDP context selected (conf_id = %d)", current_conf_id)
  }

  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (p_persistent_ctxt->modem_cid_table[i].affected_config == current_conf_id)
    {
      /* return affected modem cid */
      return (p_persistent_ctxt->modem_cid_table[i].cid_value);
    }
  }

  PrintErr("unexpected error atcm_get_affected_modem_cid()")
  return (1U); /* return first valid cid by default (do not return 0 which is a special cid value !!!) */
}

/*
*  Get user Config ID corresponding to this modem cid
*/
CS_PDN_conf_id_t atcm_get_configID_for_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, uint8_t modem_cid)
{
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    if (p_persistent_ctxt->modem_cid_table[i].cid_value == modem_cid)
    {
      /* return corresponding Used Config ID */
      return (p_persistent_ctxt->modem_cid_table[i].affected_config);
    }
  }

  PrintErr("Modem CID %d not found in the table", modem_cid)
  return (CS_PDN_NOT_DEFINED);
}

/* ==========================  Build 3GPP TS 27.007 commands ========================== */
at_status_t fCmdBuild_NoParams(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_atp_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  /* Command as no parameters - STUB function */
  PrintAPI("enter fCmdBuild_NoParams()")

  return (retval);
}

at_status_t fCmdBuild_CGSN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGSN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CGSN=<n> where <n>:
    * 0: serial number
    * 1: IMEI
    * 2: IMEISV (IMEI and Software version number)
    * 3: SVN (Software version number)
    *
    * <n> parameter is set previously
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d",  p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param);
  }
  return (retval);
}

at_status_t fCmdBuild_CMEE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CMEE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CMEE=<n> where <n>:
    * 0: <err> result code disabled and ERROR used
    * 1: <err> result code enabled and numeric <ERR> values used
    * 2: <err> result code enabled and verbose <ERR> values used
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->persist.cmee_level);
  }
  return (retval);
}

at_status_t fCmdBuild_CPIN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CPIN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    PrintDBG("pin code= %s", p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode)

    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%s", p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode);
  }
  return (retval);
}

at_status_t fCmdBuild_CFUN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CFUN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == SID_CS_INIT_MODEM)
    {
      uint8_t fun = 1U, rst = 0U;
      /* at modem init, get CFUN mode from parameters */
      csint_modemInit_t *modemInit_struct = &(p_modem_ctxt->SID_ctxt.modem_init);

      /* AT+CFUN=<fun>,<rst>
      * where <fun>:
      *  0: minimum functionality
      *  1: full functionality
      *  4: disable phone TX & RX
      * where <rst>:
      *  0: do not reset modem before setting <fun> parameter
      *  1: reset modem before setting <fun> parameter
      */

      /* convert cellular service paramaters to Modem format */
      if (modemInit_struct->init == CS_CMI_MINI)
      {
        fun = 0U;
      }
      else if (modemInit_struct->init == CS_CMI_FULL)
      {
        fun = 1U;
      }
      else if (modemInit_struct->init == CS_CMI_SIM_ONLY)
      {
        fun = 4U;
      }
      else
      {
        PrintErr("invalid parameter")
        return (ATSTATUS_ERROR);
      }

      (modemInit_struct->reset == CELLULAR_TRUE) ? (rst = 1U) : (rst = 0U);

      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,%d", fun, rst);
    }
    else
    {
      /* set parameter defined by user */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->CMD_ctxt.cfun_value);
    }



  }
  return (retval);
}

at_status_t fCmdBuild_COPS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_COPS()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_OperatorSelector_t *operatorSelect = &(p_modem_ctxt->SID_ctxt.write_operator_infos);

    if (operatorSelect->mode == CS_NRM_AUTO)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else if (operatorSelect->mode == CS_NRM_MANUAL)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "1,%d,%s", operatorSelect->format, operatorSelect->operator_name);
    }
    else if (operatorSelect->mode == CS_NRM_DEREGISTER)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (operatorSelect->mode == CS_NRM_MANUAL_THEN_AUTO)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "4,%d,%s", operatorSelect->format, operatorSelect->operator_name);
    }
    else
    {
      PrintErr("invalid mode value for +COPS")
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_CGATT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGATT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* if cgatt set by user or if in ATTACH sequence */
    if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_ATTACHED) ||
        (p_atp_ctxt->current_SID == SID_ATTACH_PS_DOMAIN))
    {
      /* request attach */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "1");
    }
    /* if cgatt set by user or if in DETACH sequence */
    else if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_DETACHED) ||
             (p_atp_ctxt->current_SID == SID_DETACH_PS_DOMAIN))
    {
      /* request detach */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    /* not in ATTACH or DETACH sequence and cgatt_write_cmd_param not set by user: error ! */
    else
    {
      PrintErr("CGATT state parameter not set")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CREG:2, will be sorted at cellular service level */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CGREG:2, will be sorted at cellular service level */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CEREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CEREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CEREG:2, will be sorted at cellular service level */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGEREP(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGEREP()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Packet Domain event reporting +CGEREP
     * 3GPP TS 27.007
     * Write command: +CGEREP=[<mode>[,<bfr>]]
     * where:
    *  <mode> = 0: no URC (+CGEV) are reported
    *         = 1: URC are discsarded when link is reserved (data on) and forwared otherwise
    *         = 2: URC are buffered when link is reserved and send when link freed, and forwared otherwise
    *  <bfr>  = 0: MT buffer of URC is cleared when <mode> 1 or 2 is entered
    *         = 1: MT buffer of URC is flushed to TE when <mode> 1 or 2 is entered
    */
    if (p_atp_ctxt->current_SID == SID_CS_REGISTER_PDN_EVENT)
    {
      /* enable notification (hard-coded value 1,0) */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "1,0");
    }
    else if (p_atp_ctxt->current_SID == SID_CS_DEREGISTER_PDN_EVENT)
    {
      /* disable notifications */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* nothing to do */
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGDCONT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGDCONT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {

    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id != CS_PDN_NOT_DEFINED)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\"",
              modem_cid,
              atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type),
              p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn);
    }
    else
    {
      PrintErr("PDP context not defined for conf_id %d", current_conf_id)
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGACT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* PDP context activate or deactivate
    *  3GPP TS 27.007
    *  AT+CGACT=[<state>[,<cid>[,<cid>[,...]]]]
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d,%d",
            ((p_modem_ctxt->CMD_ctxt.pdn_state == AT_TRUE) ? 1 : 0),
            modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_CGDATA(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGDATA()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* Enter data state
    *  3GPP TS 27.007
    *  AT+CGDATA[=<L2P>[,<cid>[,<cid>[,...]]]]
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "\"PPP\",%d",
            modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_CGPADDR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGPADDR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* Show PDP adress(es)
    *  3GPP TS 27.007
    *  AT+CGPADDR[=<cid>[,<cid>[,...]]]
    *
    *  implemenation: we only request address for 1 cid (if more cid required, call it again)
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
  }

  return (retval);
}

/* ==========================  Build V.25ter commands ========================== */
at_status_t fCmdBuild_ATD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATD()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* actually implemented specifically for each modem
    *  following example is not guaranteed ! (cid is not specified here)
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "*99#");
  }
  return (retval);
}

at_status_t fCmdBuild_ATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATE()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.command_echo == AT_TRUE)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_ATV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATV()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.dce_full_resp_format == AT_TRUE)
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_ATX(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATX()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* CONNECT Result code and monitor call progress
    *  for the moment, ATX0 to return result code only, dial tone and busy detection are both disabled
    */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");
  }
  return (retval);
}

at_status_t fCmdBuild_IPR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_IPR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set baud rate */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%ld", p_modem_ctxt->CMD_ctxt.baud_rate);
  }
  return (retval);
}

at_status_t fCmdBuild_IFC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_IPR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set flow control */
    if (p_modem_ctxt->CMD_ctxt.flow_control_cts_rts == AT_FALSE)
    {
      /* No flow control */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "0,0");
    }
    else
    {
      /* CTS/RTS activated */
      sprintf((char *)p_atp_ctxt->current_atcmd.params, "2,2");
    }
  }
  return (retval);
}


at_status_t fCmdBuild_ESCAPE_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ESCAPE_CMD()")

  /* only for RAW command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    /* set escape sequence (as define in custom modem specific) */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "%s", p_atp_ctxt->current_atcmd.name);
    /* set raw command size */
    p_atp_ctxt->current_atcmd.raw_cmd_size = strlen((char *)p_atp_ctxt->current_atcmd.params);
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

at_status_t fCmdBuild_AT_AND_D(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /*UNUSED(p_modem_ctxt);*/

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_AT_AND_D()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* Set DTR function mode  (cf V.25ter)
     * hard-coded to 0
     */
    sprintf((char *)p_atp_ctxt->current_atcmd.params, "0");

  }

  return (retval);
}
/* ==========================  Analyze 3GPP TS 27.007 commands ========================== */
at_action_rsp_t fRspAnalyze_None(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_None()")

  /* no parameters expected */

  return (retval);
}

at_action_rsp_t fRspAnalyze_Error(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PrintAPI("enter fRspAnalyze_Error()")

  /* analyse parameters for ERROR */
  /* use CmeErr function for the moment */
  retval = fRspAnalyze_CmeErr(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  return (retval);
}

at_action_rsp_t fRspAnalyze_CmeErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PrintAPI("enter fRspAnalyze_CmeErr()")

  /* Analyze CME error to report it to upper layers */
  analyze_CmeError(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  /* specific treatments for +CME ERROR, depending of current command */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case _AT_CGSN:
      switch (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param)
      {
        case CGSN_SN:
          PrintDBG("Modem Error for CGSN_SN, use unitialized value")
          memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number), '0', MAX_SIZE_SN);
          break;

        case CGSN_IMEI:
          PrintDBG("Modem Error for CGSN_IMEI, use unitialized value")
          memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei), '0', MAX_SIZE_IMEI);
          break;

        case CGSN_IMEISV:
          PrintDBG("Modem Error for CGSN_IMEISV, use unitialized value, parameter ignored")
          break;

        case CGSN_SVN:
          PrintDBG("Modem Error for CGSN_SVN, use unitialized value, parameter ignored")
          break;

        default:
          PrintDBG("Modem Error for CGSN, unexpected parameter")
          retval = ATACTION_RSP_ERROR;
          break;
      }
      break;

    case _AT_CIMI:
      PrintDBG("Modem Error for CIMI, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imsi), '0', MAX_SIZE_IMSI);
      break;

    case _AT_CGMI:
      PrintDBG("Modem Error for CGMI, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.manufacturer_name), '0', MAX_SIZE_MANUFACT_NAME);
      break;

    case _AT_CGMM:
      PrintDBG("Modem Error for CGMM, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.model), '0', MAX_SIZE_MODEL);
      break;

    case _AT_CGMR:
      PrintDBG("Modem Error for CGMR, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.revision), '0', MAX_SIZE_REV);
      break;

    case _AT_CNUM:
      PrintDBG("Modem Error for CNUM, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.phone_number), '0', MAX_SIZE_PHONE_NBR);
      break;

    case _AT_GSN:
      PrintDBG("Modem Error for GSN, use unitialized value")
      memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei), '0', MAX_SIZE_IMEI);
      break;

    case _AT_CPIN:
      PrintDBG("Analyze Modem Error for CPIN")
      break;

    /* consider all other error cases for AT commands
     * case ?:
     * etc...
    */

    default:
      PrintDBG("Modem Error for cmd (id=%ld)", p_atp_ctxt->current_atcmd.id)
      retval = ATACTION_RSP_ERROR;
      break;

  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CmsErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CmsErr()")

  /* analyze parameters for +CMS ERROR */
  /* Not implemented */

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMI()")

  /* analyze parameters for +CGMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Manufacturer name:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.manufacturer_name),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMM()")

  /* analyze parameters for +CGMM */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Model:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.model),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMR()")

  /* analyze parameters for +CGMR */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Revision:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.revision),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGSN()")

  /* analyze parameters for +CGSN */
  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SN)
    {
      /* serial number */
      PrintDBG("Serial Number:")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

      memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEI)
    {
      /* IMEI */
      PrintDBG("IMEI:")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

      memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEISV)
    {
      /* IMEISV */
      PrintDBG("IMEISV (NOT USED):")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SVN)
    {
      /* SVN */
      PrintDBG("SVN (NOT USED):")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    else
    {
      /* nothing to do */
    }
  }

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Serial Number:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CIMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CIMI()")

  /* analyze parameters for +CIMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("IMSI:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imsi),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CEER(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CEER()")

  /* analyze parameters for CEER */

  return (retval);
}

at_action_rsp_t fRspAnalyze_CPIN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CPIN()")

  /* analyze parameters for CPIN */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PrintDBG("CPIN parameter received:")
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
    if ((char *) strstr((const char *)&line[0], "SIM PIN") != NULL)
    {
      PrintDBG("waiting for SIM PIN")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PUK") != NULL)
    {
      PrintDBG("waiting for SIM PUK")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PIN2") != NULL)
    {
      PrintDBG("waiting for SIM PUK2")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "SIM PUK2") != NULL)
    {
      PrintDBG("waiting for SIM PUK")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((char *) strstr((const char *)&line[0], "READY") != NULL)
    {
      PrintDBG("CPIN READY")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_TRUE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_READY;
    }
    else
    {
      PrintErr("UNEXPECTED CPIN STATE")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
  }
  END_PARAM_LOOP()
  return (retval);
}

at_action_rsp_t fRspAnalyze_COPS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_COPS()")

  /* analyze parameters for +COPS
  *  Different cases to consider (as format is different)
  *  1/ answer to COPS read command
  *     +COPS: <mode>[,<format>,<oper>[,<AcT>]]
  *  2/ answer to COPS test command
  *     +COPS: [list of supported (<stat>,long alphanumeric <oper>,
  *            short alphanumeric <oper>,numeric <oper>[,<AcT>])s]
  *            [,,(list ofsupported <mode>s),(list of supported <format>s)]
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* mode (mandatory) */
      uint32_t mode = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      switch (mode)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_DEREGISTER;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL_THEN_AUTO;
          break;
        default:
          PrintErr("invalid mode value in +COPS")
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
      }

      PrintDBG("+COPS: mode = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.mode)
    }
    else if (element_infos->param_rank == 3U)
    {
      /* format (optional) */
      uint32_t format = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_FORMAT_PRESENT;
      switch (format)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_LONG;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_SHORT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NUMERIC;
          break;
        default:
          PrintErr("invalid format value")
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NOT_PRESENT;
          break;
      }
      PrintDBG("+COPS: format = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.format)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* operator name (optional) */
      if (element_infos->str_size > MAX_SIZE_OPERATOR_NAME)
      {
        PrintErr("error, operator name too long")
        return (ATACTION_RSP_ERROR);
      }
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_OPERATOR_NAME_PRESENT;
      memcpy((void *) & (p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name[0]),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
      PrintDBG("+COPS: operator name = %s", p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name)
    }
    else if (element_infos->param_rank == 5U)
    {
      /* AccessTechno (optional) */
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_ACT_PRESENT;
      uint32_t AcT = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      switch (AcT)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_COMPACT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN;
          break;
        case 3:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_EDGE;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA;
          break;
        case 5:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSUPA;
          break;
        case 6:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA_HSUPA;
          break;
        case 7:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN;
          break;
        case 8:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_EC_GSM_IOT;
          PrintINFO(">>> Access Technology : LTE Cat.M1 <<<")
          break;
        case 9:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN_NBS1;
          PrintINFO(">>> Access Technology : LTE Cat.NB1 <<<")
          break;
        default:
          PrintErr("invalid AcT value")
          break;
      }

      PrintDBG("+COPS: Access technology = %ld", AcT)

    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PrintDBG("+COPS for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CNUM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/
  /*UNUSED(p_modem_ctxt);*/
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fCmdBuild_CNUM()")

  PrintDBG("+CNUM cmd NOT IMPLEMENTED")

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGATT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGATT()")

  /* analyze parameters for +CGATT
  *  Different cases to consider (as format is different)
  *  1/ answer to CGATT read command
  *     +CGATT: <state>
  *  2/ answer to CGATT test command
  *     +CGATT: (list of supported <state>s)
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t attach = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->SID_ctxt.attach_status = (attach == 1U) ? CS_PS_ATTACHED : CS_PS_DETACHED;
      PrintDBG("attach status = %d", p_modem_ctxt->SID_ctxt.attach_status)
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PrintDBG("+CGATT for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CREG read command
  *     +CREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *  2/ answer to CREG test command
  *     +CREG: (list of supported <n>s)
  *  3/ URC:
  *     +CREG: <stat>[,[<lac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == _AT_CREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat);
        PrintDBG("+CREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_location_info.lac = (uint8_t)lac;
        PrintDBG("+CREG: lac=%ld", lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_location_info.ci = (uint16_t)ci;
        PrintDBG("+CREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }

  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat);
      PrintDBG("+CREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.lac = (uint8_t)lac;
      PrintDBG("+CREG URC: lac=%ld", lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.ci = (uint16_t)ci;
      PrintDBG("+CREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CGREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CGREG read command
  *     +CGREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>, [<rac>] [,<cause_type>,<reject_cause>]]]
  *  2/ answer to CGREG test command
  *     +CGREG: (list of supported <n>s)
  *  3/ URC:
  *     +CGREG: <stat>[,[<lac>],[<ci>],[<AcT>],[<rac>]]
  */
  if (p_atp_ctxt->current_atcmd.id == _AT_CGREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CGREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat);
        PrintDBG("+CGREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_location_info.lac = (uint8_t)lac;
        PrintDBG("+CGREG: lac=%ld", lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_location_info.ci = (uint16_t)ci;
        PrintDBG("+CGREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CGREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 7U)
      {
        /* param traced only */
        PrintDBG("+CGREG: rac=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CGREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CGREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat);
      PrintDBG("+CGREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.lac = (uint8_t)lac;
      PrintDBG("+CGREG URC: lac=%ld", lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.ci = (uint16_t)ci;
      PrintDBG("+CGREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CGREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 6U)
    {
      /* param traced only */
      PrintDBG("+CGREG URC: rac=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);

}

at_action_rsp_t fRspAnalyze_CEREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CEREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CEREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CEREG read command
  *     +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *  2/ answer to CEREG test command
  *     +CEREG: (list of supported <n>s)
  *  3/ URC:
  *     +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == _AT_CEREG)
  {
    /* analyze parameters for +CEREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CEREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat);
        PrintDBG("+CEREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t tac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_location_info.lac = (uint8_t)tac;
        PrintDBG("+CEREG: tac=%ld", tac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_location_info.ci = (uint16_t)ci;
        PrintDBG("+CEREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CEREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CEREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CEREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_network_registration = AT_TRUE;
      p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat);
      PrintDBG("+CEREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t tac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_location_info_tac = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.lac = (uint8_t)tac;
      PrintDBG("+CEREG URC: tac=%ld", tac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.ci = (uint16_t)ci;
      PrintDBG("+CEREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CEREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGEV()")

  /* cf 3GPP TS 27.007 */
  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* due to parser implementation (spaces are not considered as a split character) and the format of +CGEV,
    *  we can receive an additional paramater with the PDN event name in the 1st CGEV parameter.
    *  For example:
    *    +CGEV: NW DETACH                                 => no additionnal parameter
    *    +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]       => <cid> will be present here
    *    +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]  => <PDP_type>  will be present here
    *
    * In consequence, a specific treatment is done here.

    * Here is a list of different events we can receive:
    *   +CGEV: NW DETACH
    *   +CGEV: ME DETACH
    *   +CGEV: NW CLASS <class>
    *   +CGEV: ME CLASS <class>
    *   +CGEV: NW PDN ACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]][,<WLAN_Offload>]
    *   +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN DEACT <cid>
    *   +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: ME DEACT <p_cid>, <cid>, <event_type>
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: NW MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: REJECT <PDP_type>, <PDP_addr>
    *   +CGEV: NW REACT <PDP_type>, <PDP_addr>, [<cid>]
    *
    *  We are only interested by following events:
    *   +CGEV: NW DETACH : the network has forced a Packet domain detach (all contexts)
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>] : the nw has forced a contex deactivation
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] : context deactivated
    */

    /* check that previous PDN URC has been reported
    *  if this is not the case, we can not report this one
    */
    if (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE)
    {
      PrintErr("an +CGEV URC still not reported, ignore this one")
      return (ATACTION_RSP_ERROR);
    }

    /* reset event params */
    reset_pdn_event(&p_modem_ctxt->persist);

    /* create a copy of params */
    uint8_t copy_params[MAX_CGEV_PARAM_SIZE] = {0};
    AT_CHAR_t *found;
    size_t size_mini = ATC_GET_MINIMUM_SIZE(element_infos->str_size, MAX_CGEV_PARAM_SIZE);
    memcpy((void *)copy_params,
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           size_mini);
    found = (AT_CHAR_t *)strtok((char *)copy_params, " ");
    while (found  != NULL)
    {
      /* analyze of +CGEV event received */
      if (0 == strcmp((char *)found, "NW"))
      {
        PrintDBG("<NW>")
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_NW;
      }
      else if (0 == strcmp((char *)found, "ME"))
      {
        PrintDBG("<ME>")
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_ME;
      }
      else if (0 == strcmp((char *)found, "PDN"))
      {
        PrintDBG("<PDN>")
        p_modem_ctxt->persist.pdn_event.event_scope = CGEV_EVENT_SCOPE_PDN;
      }
      else if (0 == strcmp((char *)found, "ACT"))
      {
        PrintDBG("<ACT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_ACTIVATION;
      }
      else if (0 == strcmp((char *)found, "DEACT"))
      {
        PrintDBG("<DEACT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DEACTIVATION;
      }
      else if (0 == strcmp((char *)found, "REJECT"))
      {
        PrintDBG("<REJECT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_REJECT;
      }
      else if (0 == strcmp((char *)found, "DETACH"))
      {
        PrintDBG("<DETACH>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DETACH;
        /* all PDN are concerned */
        p_modem_ctxt->persist.pdn_event.conf_id = CS_PDN_ALL;
      }
      else if (0 == strcmp((char *)found, "CLASS"))
      {
        PrintDBG("<CLASS>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_CLASS;
      }
      else if (0 == strcmp((char *)found, "MODIFY"))
      {
        PrintDBG("<MODIFY>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_MODIFY;
      }
      else
      {
        /* if falling here, this is certainly an additional paramater (cf above explanation) */
        if (p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW)
        {
          switch (p_modem_ctxt->persist.pdn_event.event_type)
          {
            case CGEV_EVENT_TYPE_DETACH:
              /*  we are in the case:
              *  +CGEV: NW DETACH
              *  => no parameter to analyze
              */
              PrintErr("No parameter expected for  NW DETACH")
              break;

            case CGEV_EVENT_TYPE_DEACTIVATION:
              if (p_modem_ctxt->persist.pdn_event.event_scope == CGEV_EVENT_SCOPE_PDN)
              {
                /* we are in the case:
                *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
                *   => parameter to analyze = <cid>
                */
                uint32_t cgev_cid = ATutil_convertStringToInt((uint8_t *)found, (uint16_t)strlen((char *)found));
                p_modem_ctxt->persist.pdn_event.conf_id = atcm_get_configID_for_modem_cid(&p_modem_ctxt->persist, (uint8_t)cgev_cid);
                PrintDBG("+CGEV modem cid=%ld (user conf Id =%d)", cgev_cid, p_modem_ctxt->persist.pdn_event.conf_id)
              }
              else
              {
                /* we are in the case:
                *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
                *   => parameter to analyze = <PDP_type>
                *
                *   skip, parameter not needed in this case
                */
              }
              break;

            default:
              PrintDBG("event type (= %d) ignored", p_modem_ctxt->persist.pdn_event.event_type)
              break;
          }
        }
        else
        {
          PrintDBG("ME events ignored")
        }
      }

      PrintDBG("(%d) ---> %s", strlen((char *)found), (uint8_t *) found)
      found = (AT_CHAR_t *)strtok(NULL, " ");
    }

    /* Indicate that a +CGEV URC has been received */
    p_modem_ctxt->persist.urc_avail_pdn_event = AT_TRUE;
  }
  else if (element_infos->param_rank == 3U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <PDP_addr> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */

      /* analyze <IP_address> and try to find a matching user cid */
      csint_ip_addr_info_t  ip_addr_info;
      memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* recopy IP address value, ignore type */
      ip_addr_info.ip_addr_type = CS_IPAT_INVALID;
      memcpy((void *) & (ip_addr_info.ip_addr_value),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
      PrintDBG("<PDP_addr>=%s", (char *)&ip_addr_info.ip_addr_value)

      /* find user cid matching this IP addr (if any) */
      p_modem_ctxt->persist.pdn_event.conf_id = find_user_cid_with_matching_ip_addr(&p_modem_ctxt->persist, &ip_addr_info);

    }
    else
    {
      PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <cid> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */
      /* CID not used: we could use it if problem with <PDP_addr> occured */
    }
    else
    {
      PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else
  {
    PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_CSQ(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CSQ()")

  /* analyze parameters for CSQ */
  /* for EXECUTION COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* 3GP TS27.007
    *  format: +CSQ: <rssi>,<ber>
    *
    *  <rssi>: integer type
    *          0  -113dBm or less
    *          1  -111dBm
    *          2...30  -109dBm to -53dBm
    *          31  -51dBm or greater
    *          99  unknown or not detectable
    *  <ber>: integer type (channel bit error rate in percent)
    *          0...7  as RXQUAL values in the table 3GPP TS 45.008
    *          99     not known ot not detectable
    */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t rssi = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ rssi=%ld", rssi)
      p_modem_ctxt->SID_ctxt.signal_quality->rssi = (uint8_t)rssi;
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t ber = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ ber=%ld", ber)
      p_modem_ctxt->SID_ctxt.signal_quality->ber = (uint8_t)ber;
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGPADDR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGPADDR()")

  /* analyze parameters for CGPADDR */
  /* for WRITE COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GP TS27.007
    *  format: +CGPADDR: <cid>[,<PDP_addr_1>[,>PDP_addr_2>]]]
    *
    *  <cid>: integer type
    *         specifies a particular PDP context definition
    *  <PDP_addr_1> and <PDP_addr_2>: string type
    *         format = a1.a2.a3.a4 for IPv4
    *         format = a1.a2.a3.a4.a5a.a6.a7.a8.a9.a10.a11.a12a.a13.a14.a15.a16 for IPv6
    */

    START_PARAM_LOOP()
    PrintDBG("+CGPADDR param_rank = %d", element_infos->param_rank)
    if (element_infos->param_rank == 2U)
    {
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CGPADDR cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if ((element_infos->param_rank == 3U) || (element_infos->param_rank == 4U))
    {
      /* analyze <PDP_addr_1> and <PDP_addr_2> */
      csint_ip_addr_info_t  ip_addr_info;
      memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrive IP address value */
      memcpy((void *) & (ip_addr_info.ip_addr_value),
             (void *)&p_msg_in->buffer[element_infos->str_start_idx],
             (size_t) element_infos->str_size);
      PrintDBG("+CGPADDR addr=%s", (char *)&ip_addr_info.ip_addr_value)

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
      PrintDBG("+CGPADDR nbr of points in address %d", count_dots)
      if (count_dots == 3U)
      {
        ip_addr_info.ip_addr_type = CS_IPAT_IPV4;
      }
      else
      {
        ip_addr_info.ip_addr_type = CS_IPAT_IPV6;
      }

      /* save IP address infos in modem_cid_table */
      if (element_infos->param_rank == 3U)
      {
        atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
      }
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/* ==========================  Analyze V.25ter commands ========================== */
at_action_rsp_t fRspAnalyze_GSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_GSN()")

  /* analyze parameters for +GSN */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("IMEI:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei),
           (void *)&p_msg_in->buffer[element_infos->str_start_idx],
           (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_IPR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_modem_ctxt);*/
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_IPR()")

  /* analyze parameters for +IPR */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    PrintDBG("BAUD RATE:")
    if (element_infos->param_rank == 2U)
    {
      /* param trace only */
      PrintDBG("+IPR baud rate=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
  }

  return (retval);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
