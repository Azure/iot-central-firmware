/**
  ******************************************************************************
  * @file    cellular_service.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service
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
#include "string.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "at_core.h"
#include "at_datapack.h"
#include "at_util.h"
#include "sysctrl.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_MAIN, DBL_LVL_P0, "CS:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_MAIN, DBL_LVL_P1, "CS:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_MAIN, DBL_LVL_P2, "CS API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_MAIN, DBL_LVL_ERR, "CS ERROR:" format "\n\r", ## args)
#else
#define PrintINFO(format, args...)  printf("CS:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("CS:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("CS API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("CS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_CELLULAR_SERVICE */

/* Private variables ---------------------------------------------------------*/
/* Cellular service context variables */
static at_buf_t cmd_buf[ATCMD_MAX_BUF_SIZE];
static at_buf_t rsp_buf[ATCMD_MAX_BUF_SIZE];

/* Permanent variables */
static at_handle_t _Adapter_Handle;
static cellular_event_callback_t register_event_callback; /* to report idle event in Bare mode */
static __IO uint32_t eventReceived = 0U;

/* URC callbacks */
static cellular_urc_callback_t urc_eps_network_registration_callback = NULL;
static cellular_urc_callback_t urc_gprs_network_registration_callback = NULL;
static cellular_urc_callback_t urc_cs_network_registration_callback = NULL;
static cellular_urc_callback_t urc_eps_location_info_callback = NULL;
static cellular_urc_callback_t urc_gprs_location_info_callback = NULL;
static cellular_urc_callback_t urc_cs_location_info_callback = NULL;
static cellular_urc_callback_t urc_signal_quality_callback = NULL;
static cellular_ping_response_callback_t urc_ping_rsp_callback = NULL;
static cellular_pdn_event_callback_t urc_packet_domain_event_callback[CS_PDN_CONFIG_MAX + 1U] = {NULL};
static cellular_modem_event_callback_t urc_modem_event_callback = NULL;
static sysctrl_info_t modem_device_infos;  /* LTE Modem informations */

/* Non-permanent variables  */
static csint_urc_subscription_t cs_ctxt_urc_subscription =
{
  .eps_network_registration = CELLULAR_FALSE,
  .gprs_network_registration = CELLULAR_FALSE,
  .cs_network_registration = CELLULAR_FALSE,
  .eps_location_info = CELLULAR_FALSE,
  .gprs_location_info = CELLULAR_FALSE,
  .cs_location_info = CELLULAR_FALSE,
  .signal_quality = CELLULAR_FALSE,
  .packet_domain_event = CELLULAR_FALSE,
  .ping_rsp = CELLULAR_FALSE,
};
static csint_location_info_t cs_ctxt_eps_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CELLULAR_FALSE,
  .lac_updated = CELLULAR_FALSE,
};
static csint_location_info_t cs_ctxt_gprs_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CELLULAR_FALSE,
  .lac_updated = CELLULAR_FALSE,
};
static csint_location_info_t cs_ctxt_cs_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CELLULAR_FALSE,
  .lac_updated = CELLULAR_FALSE,
};
static CS_NetworkRegState_t cs_ctxt_eps_network_reg_state = CS_NRS_UNKNOWN;
static CS_NetworkRegState_t cs_ctxt_gprs_network_reg_state = CS_NRS_UNKNOWN;
static CS_NetworkRegState_t cs_ctxt_cs_network_reg_state = CS_NRS_UNKNOWN;
static csint_socket_infos_t cs_ctxt_sockets_info[CELLULAR_MAX_SOCKETS]; /* socket infos (array index = socket handle) */

/* Structures used to send datas to lower layer */
static CS_OperatorSelector_t cs_ctxt_operator;
static CS_DeviceInfo_t cs_ctxt_device_info = {0};

/* Use google as default primary DNS service */
static const CS_DnsConf_t cs_default_primary_dns_addr = { .primary_dns_addr = "8.8.8.8" };

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void CELLULAR_reset_context(void);
static void CELLULAR_reset_socket_context(void);
static CS_Status_t CELLULAR_init(void);
static void CELLULAR_idle_event_notif(void);
static void CELLULAR_urc_notif(at_buf_t *p_rsp_buf);
static CS_Status_t CELLULAR_analyze_error_report(at_buf_t *p_rsp_buf);
static CS_Status_t convert_SIM_error(csint_error_report_t *p_error_report);

static CS_Status_t perform_HW_reset(void);
static CS_Status_t perform_SW_reset(void);
static CS_Status_t perform_Factory_reset(void);

static void modem_reset_update_socket_state(void);
static void socket_init(socket_handle_t index);
static socket_handle_t socket_allocateHandle(void);
static void socket_deallocateHandle(socket_handle_t sockhandle);
static CS_Status_t socket_create(socket_handle_t sockhandle,
                                 CS_IPaddrType_t addr_type,
                                 CS_TransportProtocol_t protocol,
                                 uint16_t local_port,
                                 CS_PDN_conf_id_t cid);
static CS_Status_t socket_bind(socket_handle_t sockhandle,
                               uint16_t local_port);
static CS_Status_t socket_configure(socket_handle_t sockhandle,
                                    CS_SocketOptionLevel_t opt_level,
                                    CS_SocketOptionName_t opt_name,
                                    void *p_opt_val);
static CS_Status_t socket_configure_remote(socket_handle_t sockhandle,
                                           CS_IPaddrType_t ip_addr_type,
                                           CS_CHAR_t *p_ip_addr_value,
                                           uint16_t remote_port);
static CS_PDN_event_t convert_to_PDN_event(csint_PDN_event_desc_t event_desc);

/* Functions Definition ------------------------------------------------------*/
#if (RTOS_USED == 0)
/**
  * @brief  Check if events have been received in idle (bare mode only)
  * @note   This function has to be called peridiocally by the client in bare mode.
  * @param  event_callback client callback to call when an event occured
  */
void CS_check_idle_events(void)
{
  if (eventReceived > 0)
  {
    AT_getevent(_Adapter_Handle, &rsp_buf[0]);
    eventReceived--;
  }
}

/**
  * @brief  Initialization of Cellular Service in bare mode
  * @note   This function is used to init Cellular Service and also set the event
  *         callback which is used in bare mode only.
  * @note   !!! IMPORTANT !!! in Bare mode, event_callback will be call under interrupt
  *         The client implementation should be as short as possible
  *         (for example: no traces !!!)
  * @param  event_callback client callback to call when an event occured
  */
CS_Status_t CS_init_bare(cellular_event_callback_t event_callback)
{
  CS_Status_t retval = CELLULAR_OK;
  PrintAPI("CS_init_bare")

  retval = CELLULAR_init();

  /* save the callback (after the call to CELLULAR_init) */
  register_event_callback = event_callback;

  return (retval);
}

#else /* RTOS_USED */

/**
  * @brief  Initialization of Cellular Service in RTOS mode
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_init(void)
{
  CS_Status_t retval = CELLULAR_OK;
  PrintAPI("CS_init")

  retval = CELLULAR_init();

  return (retval);
}
#endif /* RTOS_USED */



/**
  * @brief  Configuration of modem
  * @note   Function not implemented yet
  * @note   This function is used to configure persitent modem configuration
  *         It should not be called in a normal process but only when we
  *         want to set persistent modem configuration.
  * @param  p_config Handle on configuration structure
  * @retval CS_Status_t
  */
CS_Status_t CS_modem_config(CS_ModemConfig_t *p_config)
{
  /*UNUSED(p_config);*/
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Power ON the modem
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_power_on(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_power_on")

  /* init of Cellular module */
  if (SysCtrl_power_on(DEVTYPE_MODEM_CELLULAR) == SCSTATUS_OK)
  {
    PrintDBG("Cellular hardware reset done")

    if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
    {
      err = AT_sendcmd(_Adapter_Handle, SID_CS_POWER_ON, &cmd_buf[0], &rsp_buf[0]);
      if (err == ATSTATUS_OK)
      {
        PrintDBG("Cellular started and ready")
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when power on process")
  }
  return (retval);
}

/**
  * @brief  Power OFF the modem
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_power_off(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_power_off")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_POWER_OFF, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Stopped")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during power off process")
  }
  return (retval);
}

/**
  * @brief  Check that modem connection is successfully established
  * @note   Usually, the command AT is sent and OK is expected as response
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_check_connection(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_check_connection")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_CHECK_CNX, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Modem connection OK")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error with modem connection")
  }
  return (retval);
}

/**
  * @brief  Initialize the service and configures the Modem FW functionalities
  * @note   Used to provide PIN code (if any) and modem function level.
  * @param  init Function level (MINI, FULL, SIM only).
  * @param  reset Indicates if modem reset will be applied or not.
  * @param  pin_code PIN code string.
  *
  * @retval CS_Status_t
  */
CS_Status_t CS_init_modem(CS_ModemInit_t init, CS_Bool_t reset, const CS_CHAR_t *pin_code)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_init_modem")

  csint_modemInit_t modemInit_struct;
  memset((void *)&modemInit_struct, 0, sizeof(modemInit_struct));
  modemInit_struct.init = init;
  modemInit_struct.reset = reset;
  memcpy((void *)&modemInit_struct.pincode.pincode[0],
         (const CS_CHAR_t *)pin_code,
         strlen((const char *)pin_code));

  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_INITMODEM,
                           sizeof(csint_modemInit_t),
                           (void *)&modemInit_struct) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_INIT_MODEM, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Init done succesfully")
      retval = CELLULAR_OK;
    }
    else
    {
      retval = CELLULAR_analyze_error_report(&rsp_buf[0]);
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during init")
  }
  return (retval);
}

/**
  * @brief  Return information related to modem status.
  * @param  p_devinfo Handle on modem information structure.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_device_info(CS_DeviceInfo_t *p_devinfo)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_get_device_info")

  /* reset our local copy */
  memset((void *)&cs_ctxt_device_info, 0, sizeof(cs_ctxt_device_info));
  cs_ctxt_device_info. field_requested = p_devinfo->field_requested;
  if (DATAPACK_writePtr(&cmd_buf[0], CSMT_DEVICE_INFO, (void *)&cs_ctxt_device_info) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_GET_DEVICE_INFO, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Device infos received")
      /* send info to user */
      memcpy((void *)p_devinfo, (void *)&cs_ctxt_device_info, sizeof(CS_DeviceInfo_t));
      retval = CELLULAR_OK;
    }
    else
    {
      retval = CELLULAR_analyze_error_report(&rsp_buf[0]);
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when getting device infos")
  }
  return (retval);
}

/**
  * @brief  Request the Modem to register to the Cellular Network.
  * @note   This function is used to select the operator. It returns a detailled
  *         network registration status.
  * @param  p_devinfo Handle on operator information structure.
  * @param  p_reg_status Handle on registration information structure.
  *         This information is valid only if return code is CELLULAR_OK
  * @retval CS_Status_t
  */
CS_Status_t CS_register_net(CS_OperatorSelector_t *p_operator,
                            CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_register_net")

  /* init returned fields */
  p_reg_status->optional_fields_presence = CS_RSF_NONE;
  p_reg_status->CS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->GPRS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->EPS_NetworkRegState = CS_NRS_UNKNOWN;

  memcpy((void *)&cs_ctxt_operator, (void *)p_operator, sizeof(CS_OperatorSelector_t));
  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_OPERATORSELECT,
                           sizeof(CS_OperatorSelector_t),
                           (void *)&cs_ctxt_operator) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_REGISTER_NET, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      if (DATAPACK_readStruct(&rsp_buf[0],
                              CSMT_REGISTRATIONSTATUS,
                              sizeof(CS_RegistrationStatus_t),
                              p_reg_status) == DATAPACK_OK)
      {
        PrintDBG("<Cellular_Service> Network registration done")
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during network registration ")
  }
  return (retval);
}

/**
  * @brief  Register to an event change notification related to Network status.
  * @note   This function should be called for each event the user wants to start monitoring.
  * @param  event Event that will be registered to change notification.
  * @param  urc_callback Handle on user callback that will be used to notify a
  *                      change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t CS_subscribe_net_event(CS_UrcEvent_t event, cellular_urc_callback_t urc_callback)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_subscribe_net_event")

  /* URC registration */
  if (event == CS_URCEVENT_EPS_NETWORK_REG_STAT)
  {
    urc_eps_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.eps_network_registration = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
  {
    urc_gprs_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.gprs_network_registration = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_CS_NETWORK_REG_STAT)
  {
    urc_cs_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.cs_network_registration = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_EPS_LOCATION_INFO)
  {
    urc_eps_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.eps_location_info = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_GPRS_LOCATION_INFO)
  {
    urc_gprs_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.gprs_location_info = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_CS_LOCATION_INFO)
  {
    urc_cs_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.cs_location_info = CELLULAR_TRUE;
  }
  else if (event == CS_URCEVENT_SIGNAL_QUALITY)
  {
    urc_signal_quality_callback = urc_callback;
    cs_ctxt_urc_subscription.signal_quality = CELLULAR_TRUE;
  }
  else
  {
    PrintErr("<Cellular_Service> invalid event")
    return (CELLULAR_ERROR);
  }

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_URC_EVENT, sizeof(CS_UrcEvent_t), (void *)&event) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_SUSBCRIBE_NET_EVENT, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> event suscribed")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when subscribing event")
  }
  return (retval);
}

/**
  * @brief  Deregister to an event change notification related to Network status.
  * @note   This function should be called for each event the user wants to stop monitoring.
  * @param  event Event that will be deregistered to change notification.
  * @retval CS_Status_t
  */
CS_Status_t CS_unsubscribe_net_event(CS_UrcEvent_t event)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_unsubscribe_net_event")

  if (event == CS_URCEVENT_EPS_NETWORK_REG_STAT)
  {
    urc_eps_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.eps_network_registration = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
  {
    urc_gprs_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.gprs_network_registration = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_CS_NETWORK_REG_STAT)
  {
    urc_cs_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.cs_network_registration = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_EPS_LOCATION_INFO)
  {
    urc_eps_location_info_callback = NULL;
    cs_ctxt_urc_subscription.eps_location_info = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_GPRS_LOCATION_INFO)
  {
    urc_gprs_location_info_callback = NULL;
    cs_ctxt_urc_subscription.gprs_location_info = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_CS_LOCATION_INFO)
  {
    urc_cs_location_info_callback = NULL;
    cs_ctxt_urc_subscription.cs_location_info = CELLULAR_FALSE;
  }
  else if (event == CS_URCEVENT_SIGNAL_QUALITY)
  {
    urc_signal_quality_callback = NULL;
    cs_ctxt_urc_subscription.signal_quality = CELLULAR_FALSE;
  }
  else
  {
    PrintErr("<Cellular_Service> invalid event")
    return (CELLULAR_ERROR);
  }

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_URC_EVENT, sizeof(CS_UrcEvent_t), (void *)&event) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_UNSUSBCRIBE_NET_EVENT, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> event unsubscribed")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when unsubscribing event")
  }
  return (retval);
}

/**
  * @brief  Request attach to packet domain.
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t CS_attach_PS_domain(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_attach_PS_domain")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_ATTACH_PS_DOMAIN, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_ATTACH_PS_DOMAIN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> attach PS domain done")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when attaching PS domain")
  }
  return (retval);
}

/**
  * @brief  Request detach to packet domain.
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t CS_detach_PS_domain(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_detach_PS_domain")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_DETACH_PS_DOMAIN, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_DETACH_PS_DOMAIN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> detach PS domain done")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when detaching PS domain")
  }
  return (retval);
}

/**
  * @brief  Request of packet attach status.
  * @param  p_attach Handle to PS attach status.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_attach_status(CS_PSattach_t *p_attach)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_get_attachstatus")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_GET_ATTACHSTATUS, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      if (DATAPACK_readStruct(&rsp_buf[0], CSMT_ATTACHSTATUS, sizeof(CS_PSattach_t), p_attach) == DATAPACK_OK)
      {
        PrintDBG("<Cellular_Service> Attachment status received = %d", *p_attach)
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when getting attachment status")
  }
  return (retval);
}

/**
  * @brief  Read the latest registration state to the Cellular Network.
  * @param  p_reg_status Handle to registration status structure.
  *         This information is valid only if return code is CELLULAR_OK
  * @retval CS_Status_t
  */
CS_Status_t CS_get_net_status(CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_get_netstatus")

  /* init returned fields */
  p_reg_status->optional_fields_presence = CS_RSF_NONE;
  p_reg_status->CS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->GPRS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->EPS_NetworkRegState = CS_NRS_UNKNOWN;

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_GET_NETSTATUS, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      if (DATAPACK_readStruct(&rsp_buf[0],
                              CSMT_REGISTRATIONSTATUS,
                              sizeof(CS_RegistrationStatus_t),
                              p_reg_status) == DATAPACK_OK)
      {
        PrintDBG("<Cellular_Service> Net status received")
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when getting net status")
  }
  return (retval);
}

/**
  * @brief  Read the actual signal quality seen by Modem .
  * @param  p_sig_qual Handle to signal quality structure.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_signal_quality(CS_SignalQuality_t *p_sig_qual)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_get_signal_quality")

  CS_SignalQuality_t local_sig_qual = {0};
  memset((void *)&local_sig_qual, 0, sizeof(CS_SignalQuality_t));

  if (DATAPACK_writePtr(&cmd_buf[0], CSMT_SIGNAL_QUALITY, (void *)&local_sig_qual) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_GET_SIGNAL_QUALITY, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Signal quality informations received")
      /* recopy info to user */
      memcpy((void *)p_sig_qual, (void *)&local_sig_qual, sizeof(CS_SignalQuality_t));
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when getting signal quality")
  }
  return (retval);
}

/**
  * @brief  Activates a PDN (Packet Data Network Gateway) allowing communication with internet.
  * @note   This function triggers the allocation of IP public WAN to the device.
  * @note   Only one PDN can be activated at a time.
  * @param  cid Configuration identifier
  *         This parameter can be one of the following values:
  *         CS_PDN_PREDEF_CONFIG To use default PDN configuration.
  *         CS_PDN_USER_CONFIG_1-5 To use a dedicated PDN configuration.
  *         CS_PDN_CONFIG_DEFAULT To use default PDN config set by CS_set_default_pdn().
  * @retval CS_Status_t
  */
CS_Status_t CS_activate_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_activate_pdn for cid=%d", cid)

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_ACTIVATE_PDN, sizeof(CS_PDN_conf_id_t), (void *)&cid) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_ACTIVATE_PDN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN %d connected", cid)
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when PDN %cid activation", cid)
  }
  return (retval);
}

/**
  * @brief  Deactivates a PDN.
  * @note   This function triggers the allocation of IP public WAN to the device.
  * @note  only one PDN can be activated at a time.
  * @param  cid Configuration identifier
  * @retval CS_Status_t
  */
CS_Status_t CS_deactivate_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_deactivate_pdn for cid=%d", cid)

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_DEACTIVATE_PDN, sizeof(CS_PDN_conf_id_t), (void *)&cid) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DEACTIVATE_PDN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN %d deactivated", cid)
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when PDN %cid deactivation", cid)
  }
  return (retval);
}

/**
  * @brief  Define internet data profile for a configuration identifier
  * @param  cid Configuration identifier
  * @param  apn A string of the access point name (must be non NULL)
  * @param  pdn_conf Structure which contains additional configurations parameters (if non NULL)
  * @retval CS_Status_t
  */
CS_Status_t CS_define_pdn(CS_PDN_conf_id_t cid, const CS_CHAR_t *apn, CS_PDN_configuration_t *pdn_conf)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  csint_pdn_infos_t pdn_infos;

  PrintAPI("CS_define_pdn for cid=%d", cid)

  /* check parameters validity */
  if ((cid < CS_PDN_USER_CONFIG_1) || (cid > CS_PDN_USER_CONFIG_5))
  {
    PrintErr("<Cellular_Service> selected configuration id %d can not be set by user", cid)
    return (CELLULAR_ERROR);
  }
  if (apn == NULL)
  {
    PrintErr("<Cellular_Service> apn must be non NULL")
    return (CELLULAR_ERROR);
  }

  /* prepare and send PDN infos */
  memset((void *)&pdn_infos, 0, sizeof(csint_pdn_infos_t));
  pdn_infos.conf_id = cid;
  memcpy((void *)&pdn_infos.apn[0],
         (const CS_CHAR_t *)apn,
         strlen((const char *)apn));
  memcpy((void *)&pdn_infos.pdn_conf, (void *)pdn_conf, sizeof(CS_PDN_configuration_t));

  if (DATAPACK_writePtr(&cmd_buf[0], CSMT_DEFINE_PDN, (void *)&pdn_infos) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DEFINE_PDN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN %d defined", cid)
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when defining PDN %d", cid)
  }
  return (retval);
}

/**
  * @brief  Select a PDN among of defined configuration identifier(s) as the default.
  * @note   By default, PDN_PREDEF_CONFIG is considered as the default PDN.
  * @param  cid Configuration identifier.
  * @retval CS_Status_t
  */
CS_Status_t CS_set_default_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_set_default_pdn (conf_id=%d)", cid)

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_SET_DEFAULT_PDN, sizeof(CS_PDN_conf_id_t), (void *)&cid) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_SET_DEFAULT_PDN, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN %d set as default", cid)
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when setting default PDN %d", cid)
  }
  return (retval);
}

/**
  * @brief  Get the IP address allocated to the device for a given PDN.
  * @param  cid Configuration identifier number.
  * @param  ip_addr_type IP address type and format.
  * @param  p_ip_addr_value Specifies the IP address of the given PDN.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_dev_IP_address(CS_PDN_conf_id_t cid, CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_get_dev_IP_address (for conf_id=%d)", cid)

  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_GET_IP_ADDRESS,
                           sizeof(CS_PDN_conf_id_t),
                           (void *)&cid) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_GET_IP_ADDRESS, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      csint_ip_addr_info_t ip_addr_info;
      if (DATAPACK_readStruct(&rsp_buf[0],
                              CSMT_GET_IP_ADDRESS,
                              sizeof(csint_ip_addr_info_t),
                              &ip_addr_info) == DATAPACK_OK)
      {
        PrintDBG("<Cellular_Service> IP address informations received")
        /* recopy info to user */
        *ip_addr_type = ip_addr_info.ip_addr_type;
        memcpy((void *)p_ip_addr_value,
               (void *)&ip_addr_info.ip_addr_value,
               strlen((char *)ip_addr_info.ip_addr_value));
        PrintDBG("<Cellular_Service> IP address = %s (type = %d)",
                 (CS_CHAR_t *)ip_addr_info.ip_addr_value, ip_addr_info.ip_addr_type)
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when getting IP address informations")
  }
  return (retval);
}

/**
  * @brief  Register to specified modem events.
  * @note   This function should be called once with all requested events.
  * @param  events_mask Events that will be registered (bitmask)
  * @param  urc_callback Handle on user callback that will be used to notify a
  *         change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t CS_subscribe_modem_event(CS_ModemEvent_t events_mask, cellular_modem_event_callback_t modem_evt_cb)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_subscribe_modem_event")

  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_MODEM_EVENT,
                           sizeof(CS_ModemEvent_t),
                           (void *)&events_mask) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_SUSBCRIBE_MODEM_EVENT, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      urc_modem_event_callback = modem_evt_cb;
      PrintDBG("<Cellular_Service> modem events suscribed")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when subscribing modem event")
  }
  return (retval);
}

/**
  * @brief  Register to event notification related to internet connection.
  * @note   This function is used to register to an event related to a PDN
  *         Only explicit config id (CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5) are
  *         suppported and CS_PDN_PREDEF_CONFIG
  * @param  cid Configuration identifier number.
  * @param  pdn_event_callback client callback to call when an event occured.
  * @retval CS_Status_t
  */
CS_Status_t  CS_register_pdn_event(CS_PDN_conf_id_t cid, cellular_pdn_event_callback_t pdn_event_callback)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_register_pdn_event")

  /* check parameters validity */
  if (cid > CS_PDN_USER_CONFIG_5)
  {
    PrintErr("<Cellular_Service> only explicit PDN user config is supported (cid=%d)", cid)
    return (CELLULAR_ERROR);
  }

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_REGISTER_PDN_EVENT, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN events registered successfully")
      /* register callback */
      urc_packet_domain_event_callback[cid] = pdn_event_callback;
      cs_ctxt_urc_subscription.packet_domain_event = CELLULAR_TRUE;
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service>error when registering PDN events")
  }
  return (retval);
}

/**
  * @brief  Deregister the internet event notification.
  * @param  cid Configuration identifier number.
  * @retval CS_Status_t
  */
CS_Status_t CS_deregister_pdn_event(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_deregister_pdn_event")

  /* check parameters validity */
  if (cid > CS_PDN_USER_CONFIG_5)
  {
    PrintErr("<Cellular_Service> only explicit PDN user config is supported (cid=%d)", cid)
    return (CELLULAR_ERROR);
  }

  /* register callback */
  urc_packet_domain_event_callback[cid] = NULL;
  cs_ctxt_urc_subscription.packet_domain_event = CELLULAR_FALSE;
  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DEREGISTER_PDN_EVENT, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> PDN events deregistered successfully")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when deregistering PDN events")
  }
  return (retval);
}

/**
  * @brief Autocativate PDN
  * @note  Function not implemeted yet
  * @param  cid Configuration identifier number.
  * @retval CS_Status_t
  */
CS_Status_t CS_autoactivate_pdn(void /* params TBD */)
{
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Allocate a socket among of the free sockets (maximum 6 sockets)
  * @param  addr_type Specifies a communication domain.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4 (default)
  *         IPAT_IPV6  for IPV6
  * @param  protocol Specified the transport protocol to be used with the socket.
  *         TCP_PROTOCOL
  *         UDP_PROTOCOL
  * @param  cid Specifies the identity of PDN configuration to be used.
  *         CS_PDN_PREDEF_CONFIG To use default PDN configuration.
  *         CS_PDN_USER_CONFIG_1-5 To use a dedicated PDN configuration.
  * @retval Socket handle which references allocated socket
  */
socket_handle_t CDS_socket_create(CS_IPaddrType_t addr_type,
                                  CS_TransportProtocol_t protocol,
                                  CS_PDN_conf_id_t cid)
{
  PrintAPI("CDS_socket_create")

  socket_handle_t sockhandle = socket_allocateHandle();
  if (sockhandle == CS_INVALID_SOCKET_HANDLE)
  {
    PrintErr("no free socket handle")
    return (CS_INVALID_SOCKET_HANDLE);
  }

  if (socket_create(sockhandle, addr_type, protocol, /* default local_port = 0 */ 0U, cid) != CELLULAR_OK)
  {
    /* deallocate handle */
    socket_deallocateHandle(sockhandle);
    PrintErr("socket creation failed")
    return (CS_INVALID_SOCKET_HANDLE);
  }

  PrintINFO("allocated socket handle=%d (local)", sockhandle)
  return (sockhandle);
}

/**
  * @brief  Bind the socket to a local port.
  * @note   If this function is not called, default local port value = 0 will be used.
  * @param  sockHandle Handle of the socket
  * @param  local_port Local port number.
  *         This parameter must be a value between 0 and 65535.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_bind(socket_handle_t sockHandle,
                            uint16_t local_port)
{
  PrintAPI("CDS_socket_bind")

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CREATED)
  {
    PrintErr("<Cellular_Service> socket bind allowed only after create/before connect %d ", sockHandle)
    return (CELLULAR_ERROR);
  }

  if (socket_bind(sockHandle, local_port) != CELLULAR_OK)
  {
    PrintErr("Socket Bind error")
    return (CELLULAR_ERROR);
  }

  return (CELLULAR_OK);
}

/**
  * @brief  Set the callbacks to use when datas are received or sent.
  * @note   This function has to be called before to use a socket.
  * @param  sockHandle Handle of the socket
  * @param  data_ready_cb Pointer to the callback function to call when datas are received
  * @param  data_sent_cb Pointer to the callback function to call when datas has been sent
  *         This parameter is only used for asynchronous behavior (NOT IMPLEMENTED)
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_set_callbacks(socket_handle_t sockHandle,
                                     cellular_socket_data_ready_callback_t data_ready_cb,
                                     cellular_socket_data_sent_callback_t data_sent_cb,
                                     cellular_socket_closed_callback_t remote_close_cb)
{
  PrintAPI("CDS_socket_set_callbacks")

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PrintErr("<Cellular_Service> invalid socket handle %d (set cb)", sockHandle)
    return (CELLULAR_ERROR);
  }

  cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback = data_ready_cb;
  /*PrintDBG("DBG: socket data ready callback=%p", cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback)*/
  if (data_ready_cb == NULL)
  {
    PrintErr("data_ready_cb is mandatory")
    return (CELLULAR_ERROR);
  }

  cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback = remote_close_cb;
  /*PrintDBG("DBG: socket remote closed callback=%p", cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback)*/
  if (remote_close_cb == NULL)
  {
    PrintErr("remote_close_cb is mandatory")
    return (CELLULAR_ERROR);
  }

  if (data_sent_cb != NULL)
  {
    /* NOT SUPPORTED */
    PrintErr("DATA sent callback not supported (only synch mode)")
  }

  return (CELLULAR_OK);
}

/**
  * @brief  Define configurable options for a created socket.
  * @note   This function is called to configure one parameter at a time.
  *         If a parameter is not configured with this function, a default value will be applied.
  * @param  sockHandle Handle of the socket
  * @param  opt_level The level of TCP/IP stack component to be configured.
  *         This parameter can be one of the following values:
  *         SOL_IP
  *         SOL_TRANSPORT
  * @param  opt_name
  *         SON_IP_MAX_PACKET_SIZE Maximum packet size for data transfer (0 to 1500 bytes).
  *         SON_TRP_MAX_TIMEOUT Inactivity timeout (0 to 65535, in second, 0 means infinite).
  *         SON_TRP_CONNECT_SETUP_TIMEOUT Maximum timeout to setup connection with remote server
  *              (10 to 1200, in 100 of ms, 0 means infinite).
  *         SON_TRP_TRANSFER_TIMEOUT Maximum timeout to transfer data to remote server
  *              (1 to 255, in ms, 0 means infinite).
  *         SON_TRP_CONNECT_MODE To indicate if connection with modem will be dedicated (CM_ONLINE_MODE)
  *             or stay in command mode (CM_COMMAND_MODE) or stay in online mode until a
  *             suspend timeout expires (CM_ONLINE_AUTOMATIC_SUSPEND).
  *             Only CM_COMMAND_MODE is supported for the moment.
  *         SON_TRP_SUSPEND_TIMEOUT To define inactivty timeout to suspend online mode
  *             (0 to 2000, in ms , 0 means infinite).
  *         SON_TRP_RX_TIMEOUT Maximum timeout to receive data from remoteserver
  *             (0 to 255, in ms, 0 means infinite).
  * @param  p_opt_val Pointer to parameter to update
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_set_option(socket_handle_t sockHandle,
                                  CS_SocketOptionLevel_t opt_level,
                                  CS_SocketOptionName_t opt_name,
                                  void *p_opt_val)
{
  CS_Status_t retval;
  PrintAPI("CDS_socket_set_option")

  retval = socket_configure(sockHandle, opt_level, opt_name, p_opt_val);
  return (retval);
}

/**
  * @brief  Retrieve configurable options for a created socket.
  * @note   This function is called for one parameter at a time.
  * @note   Function not implemented yet
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_get_option(void /* params TBD */)
{
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Connect to a remote server (for socket client mode).
  * @note   This function is blocking until the connection is setup or when the timeout to wait
  *         for socket connection expires.
  * @param  sockHandle Handle of the socket.
  * @param  ip_addr_type Specifies the type of IP address of the remote server.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4 (default)
  *         IPAT_IPV6  for IPV6
  * @param  p_ip_addr_value Specifies the IP address of the remote server.
  * @param  remote_port Specifies the port of the remote server.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_connect(socket_handle_t sockHandle,
                               CS_IPaddrType_t ip_addr_type,
                               CS_CHAR_t *p_ip_addr_value,
                               uint16_t remote_port)
{
  at_status_t err;
  CS_Status_t retval;

  PrintAPI("CDS_socket_connect")

  retval = socket_configure_remote(sockHandle, ip_addr_type, p_ip_addr_value, remote_port);
  if (retval == CELLULAR_OK)
  {
    /* Send socket informations to ATcustom
    * no need to test sockHandle validity, it has been tested in socket_configure_remote()
    */
    csint_socket_infos_t *socket_infos = &cs_ctxt_sockets_info[sockHandle];
    if (DATAPACK_writePtr(&cmd_buf[0], CSMT_SOCKET_INFO, (void *)socket_infos) == DATAPACK_OK)
    {
      if (socket_infos->trp_connect_mode == CS_CM_COMMAND_MODE)
      {
        err = AT_sendcmd(_Adapter_Handle, SID_CS_DIAL_COMMAND, &cmd_buf[0], &rsp_buf[0]);
      }
      else
      {
        /* NOT SUPPORTED YET */
        err = ATSTATUS_ERROR;
        /* err = AT_sendcmd(_Adapter_Handle, SID_CS_DIAL_ONLINE, &cmd_buf[0], &rsp_buf[0]);*/
      }

      if (err == ATSTATUS_OK)
      {
        /* update socket state */
        cs_ctxt_sockets_info[sockHandle].state = SOCKETSTATE_CONNECTED;
        retval = CELLULAR_OK;
      }
      else
      {
        PrintErr("<Cellular_Service> error when socket connection")
        retval = CELLULAR_ERROR;
      }
    }
    else
    {
      retval = CELLULAR_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Listen to clients (for socket server mode).
  * @note   Function not implemeted yet
  * @param  sockHandle Handle of the socket
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_listen(socket_handle_t sockHandle /* params TBD */)
{
  /*UNUSED(sockHandle);*/

  /* for socket server mode */
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Send data over a socket to a remote server.
  * @note   This function is blocking until the data is transfered or when the
  *         timeout to wait for transmission expires.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to transfer.
  * @param  length Length of the data buffer.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_send(socket_handle_t sockHandle,
                            const CS_CHAR_t *p_buf,
                            uint32_t length)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;

  PrintAPI("CDS_socket_send (buf@=%p - buflength = %ld)", p_buf, length)

  /* check that size does not exceed maximum buffers size */
  if (length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PrintErr("<Cellular_Service> buffer size %ld exceed maximum value %d",
             length,
             DEFAULT_IP_MAX_PACKET_SIZE)
    return (CELLULAR_ERROR);
  }

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PrintErr("<Cellular_Service> socket not connected (state=%d) for handle %d (send)",
             cs_ctxt_sockets_info[sockHandle].state,
             sockHandle)
    return (CELLULAR_ERROR);
  }

  csint_socket_data_buffer_t send_data_struct;
  send_data_struct.socket_handle = sockHandle;
  send_data_struct.p_buffer_addr_send = p_buf;
  send_data_struct.p_buffer_addr_rcv = NULL;
  send_data_struct.buffer_size = length;
  send_data_struct.max_buffer_size = length;
  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_SOCKET_DATA_BUFFER,
                           sizeof(csint_socket_data_buffer_t),
                           (void *)&send_data_struct) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_SEND_DATA, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> socket data sent")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when sending data to socket")
  }
  return (retval);
}

/**
  * @brief  Receive data from the connected remote server.
  * @note   This function is blocking until expected data length is received or a receive timeout has expired.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to received data.
  * @param  max_buf_length Maximum size of receive data buffer.
  * @retval Size of received data (in bytes).
  */
int32_t CDS_socket_receive(socket_handle_t sockHandle,
                           CS_CHAR_t *p_buf,
                           uint32_t max_buf_length)
{
  CS_Status_t status = CELLULAR_ERROR;
  at_status_t err;
  uint32_t bytes_received = 0U;

  PrintAPI("CDS_socket_receive")

  /* check that size does not exceed maximum buffers size */
  if (max_buf_length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PrintErr("<Cellular_Service> buffer size %ld exceed maximum value %d",
             max_buf_length,
             DEFAULT_IP_MAX_PACKET_SIZE)
    return (-1);
  }

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PrintErr("<Cellular_Service> socket not connected (state=%d) for handle %d (rcv)",
             cs_ctxt_sockets_info[sockHandle].state,
             sockHandle)
    return (-1);
  }

  csint_socket_data_buffer_t receive_data_struct = {0};
  receive_data_struct.socket_handle = sockHandle;
  receive_data_struct.p_buffer_addr_send = NULL;
  receive_data_struct.p_buffer_addr_rcv = p_buf;
  receive_data_struct.buffer_size = 0U;
  receive_data_struct.max_buffer_size = max_buf_length;
  if (DATAPACK_writeStruct(&cmd_buf[0],
                           CSMT_SOCKET_DATA_BUFFER,
                           sizeof(csint_socket_data_buffer_t),
                           (void *)&receive_data_struct) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_RECEIVE_DATA, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      if (DATAPACK_readStruct(&rsp_buf[0], CSMT_SOCKET_RXDATA, sizeof(uint32_t), &bytes_received) == DATAPACK_OK)
      {
        status = CELLULAR_OK;
      }
    }
  }

  if (status == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when receiving data from socket")
    return (-1);
  }
  else
  {
    PrintINFO("Size of data received on the socket= %ld bytes", bytes_received)
    return ((int32_t)bytes_received);
  }
}

/**
  * @brief  Free a socket handle.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @param  sockHandle Handle of the socket
  * @param  force Force to free the socket.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_close(socket_handle_t sockHandle, uint8_t force)
{
  /*UNUSED(force);*/
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;

  PrintAPI("CDS_socket_close")

  switch (cs_ctxt_sockets_info[sockHandle].state)
  {
    case SOCKETSTATE_CONNECTED:
    {
      /* Send socket informations to ATcustom */
      csint_socket_infos_t *socket_infos = &cs_ctxt_sockets_info[sockHandle];
      if (DATAPACK_writePtr(&cmd_buf[0], CSMT_SOCKET_INFO, (void *)socket_infos) == DATAPACK_OK)
      {
        err = AT_sendcmd(_Adapter_Handle, SID_CS_SOCKET_CLOSE, &cmd_buf[0], &rsp_buf[0]);
        if (err == ATSTATUS_OK)
        {
          /* deallocate socket handle and reinit socket parameters */
          socket_deallocateHandle(sockHandle);
          retval = CELLULAR_OK;
        }
      }
      break;
    }

    case SOCKETSTATE_CREATED:
      PrintINFO("<Cellular_Service> socket was not connected ")
      /* deallocate socket handle and reinit socket parameters */
      socket_deallocateHandle(sockHandle);
      retval = CELLULAR_OK;
      break;

    case SOCKETSTATE_NOT_ALLOC:
      PrintErr("<Cellular_Service> invalid socket handle %d (close)", sockHandle)
      retval = CELLULAR_ERROR;
      break;

    case SOCKETSTATE_ALLOC_BUT_INVALID:
      PrintINFO("<Cellular_Service> invalid socket state (after modem reboot) ")
      /* deallocate socket handle and reinit socket parameters */
      socket_deallocateHandle(sockHandle);
      retval = CELLULAR_OK;
      break;

    default:
      PrintErr("<Cellular_Service> invalid socket state %d (close)", cs_ctxt_sockets_info[sockHandle].state)
      retval = CELLULAR_ERROR;
      break;
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when closing socket")
  }
  return (retval);
}

/**
  * @brief  Get connection status for a given socket.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @param  sockHandle Handle of the socket
  * @param  p_infos Pointer of infos structure.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_cnx_status(socket_handle_t sockHandle,
                                  CS_SocketCnxInfos_t *p_infos)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CDS_socket_cnx_status")

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PrintErr("<Cellular_Service> socket not connected (state=%d) for handle %d (status)",
             cs_ctxt_sockets_info[sockHandle].state,
             sockHandle)
    return (CELLULAR_ERROR);
  }

  /* Send socket informations to ATcustom */
  csint_socket_cnx_infos_t socket_cnx_infos;
  socket_cnx_infos.socket_handle = sockHandle;
  socket_cnx_infos.infos = p_infos;
  memset((void *)p_infos, 0, sizeof(CS_SocketCnxInfos_t));
  if (DATAPACK_writePtr(&cmd_buf[0], CSMT_SOCKET_CNX_STATUS, (void *)&socket_cnx_infos) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_SOCKET_CNX_STATUS, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> socket cnx status received")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when requesting socket cnx status")
  }
  return (retval);
}

/**
  * @brief  Request to suspend DATA mode.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_suspend_data(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_suspend_data")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DATA_SUSPEND, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> DATA suspended")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when suspending DATA")
  }
  return (retval);
}

/**
  * @brief  Request to resume DATA mode.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_resume_data(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_resume_data")

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_NONE, 0U, NULL) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DATA_RESUME, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> DATA resumed")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error when resuming DATA")
  }
  return (retval);
}

/**
  * @brief  Request to reset the device.
  * @param  rst_type Type of reset requested (SW, HW, ...)
  * @retval CS_Status_t
  */
CS_Status_t CS_reset(CS_Reset_t rst_type)
{
  CS_Status_t retval = CELLULAR_OK;
  PrintAPI("CS_reset")

  /* reset Cellular Service context */
  CELLULAR_reset_context();

  /* update sockets states */
  modem_reset_update_socket_state();

  /* reset AT context */
  if (AT_reset_context(_Adapter_Handle) != ATSTATUS_OK)
  {
    PrintErr("<Cellular_Service> Reset context error")
    retval = CELLULAR_ERROR;
  }

  switch (rst_type)
  {
    case CS_RESET_HW:
      /* perform hardware reset */
      if (perform_HW_reset() != CELLULAR_OK)
      {
        retval = CELLULAR_ERROR;
      }
      break;

    case CS_RESET_SW:
      /* perform software reset */
      if (perform_SW_reset() != CELLULAR_OK)
      {
        retval = CELLULAR_ERROR;
      }
      break;

    case CS_RESET_AUTO:
      /* perform software reset first */
      if (perform_SW_reset() != CELLULAR_OK)
      {
        /* if software reset failed, perform hardware reset */
        if (perform_HW_reset() != CELLULAR_OK)
        {
          retval = CELLULAR_ERROR;
        }
      }
      break;

    case CS_RESET_FACTORY_RESET:
      /* perform factory reset  */
      if (perform_Factory_reset() != CELLULAR_OK)
      {
        retval = CELLULAR_ERROR;
      }
      break;

    default:
      PrintErr("Invalid reset type")
      retval = CELLULAR_ERROR;
      break;
  }

  return (retval);
}

/*
* CS_dns_config() - WILL BE IMPLEMENTED LATER
* will need to update CS_DnsReq_t to remove primary_dns_addr parameter
*/
/**
  * @brief  DNS configuration
  * @note   Configure the DNS
  * @param  cid Configuration identifier number.
  * @param  *dns_conf Handle to DNS configuration structure.
  * @retval CS_Status_t
  */

CS_Status_t CS_dns_config(CS_PDN_conf_id_t cid, CS_DnsConf_t *dns_conf)
{
  CS_Status_t retval = CELLULAR_ERROR;
  /* at_status_t err; */
  PrintAPI("CS_dns_config")

  /* NOT IMPLEMENTED YET
  *  Offer the possibilty to user to configure the DNS primary address.
  *  For the moment, it is hard-coded in this file.
  */

  PrintErr("<Cellular_Service> CS_dns_config not implemented yet: use CS_dns_request to config DNS")
  return (retval);
}

/**
  * @brief  DNS request
  * @note   Get IP address of the specifed hostname
  * @param  cid Configuration identifier number.
  * @param  *dns_req Handle to DNS request structure.
  * @param  *dns_resp Handle to DNS response structure.
  * @retval CS_Status_t
  */
CS_Status_t CS_dns_request(CS_PDN_conf_id_t cid, CS_DnsReq_t *dns_req, CS_DnsResp_t *dns_resp)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("CS_dns_request")

  /* build internal structure to send */
  csint_dns_request_t loc_dns_req;
  memset((void *)&loc_dns_req, 0, sizeof(csint_dns_request_t));
  /* set CID */
  loc_dns_req.conf_id = cid;
  /* recopy dns_req */
  memcpy((void *)&loc_dns_req.dns_req, dns_req, sizeof(CS_DnsReq_t));
  /* set DNS primary address to use - for the moment, it is hard-coded */
  memcpy((void *)&loc_dns_req.dns_conf,
         (const CS_DnsConf_t *)&cs_default_primary_dns_addr,
         sizeof(CS_DnsConf_t));

  if (DATAPACK_writePtr(&cmd_buf[0], CSMT_DNS_REQ, (void *)&loc_dns_req) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_DNS_REQ, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      if (DATAPACK_readStruct(&rsp_buf[0], CSMT_DNS_REQ, sizeof(dns_resp->host_addr), dns_resp->host_addr) == DATAPACK_OK)
      {
        PrintDBG("<Cellular_Service> DNS configuration done")
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during DNS request")
  }
  return (retval);
}


/**
  * @brief  Ping ip address on the network
  * @note   Usually, the command AT is sent and OK is expected as response
  * @param  address or full paramaters set
  * @param  ping_callback Handle on user callback that will be used to analyze
  *                              the ping answer from the network address.
  * @retval CS_Status_t
  */

CS_Status_t CDS_ping(CS_PDN_conf_id_t cid, CS_Ping_params_t *ping_params, cellular_ping_response_callback_t cs_ping_rsp_cb)
{
  at_status_t err;
  CS_Status_t retval = CELLULAR_ERROR;
  PrintAPI("CS_ping_address")

  /* build internal structure to send */
  csint_ping_params_t loc_ping_params;
  memset((void *)&loc_ping_params, 0, sizeof(csint_ping_params_t));
  loc_ping_params.conf_id = cid;
  memcpy((void *)&loc_ping_params.ping_params, ping_params, sizeof(CS_Ping_params_t));

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_PING_ADDRESS, sizeof(csint_ping_params_t), (void *)&loc_ping_params) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_PING_IP_ADDRESS, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      /* save the callback */
      urc_ping_rsp_callback = cs_ping_rsp_cb;
      PrintDBG("<Cellular_Service> Ping address OK")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during ping request")
  }
  return (retval);
}

/* Private function Definition -----------------------------------------------*/
static CS_Status_t perform_HW_reset(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("perform_HW_reset")

  CS_Reset_t rst_type = CS_RESET_HW;

  if (SysCtrl_reset_device(DEVTYPE_MODEM_CELLULAR) != SCSTATUS_OK)
  {
    return (CELLULAR_ERROR);
  }

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_RESET, sizeof(CS_Reset_t), (void *)&rst_type) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_RESET, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> HW device reset done")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during HW device reset")
  }
  return (retval);
}

static CS_Status_t perform_SW_reset(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("perform_SW_reset")

  CS_Reset_t rst_type = CS_RESET_SW;

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_RESET, sizeof(CS_Reset_t), (void *)&rst_type) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_RESET, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> SW device reset done")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during SW device reset")
  }
  return (retval);
}

static CS_Status_t perform_Factory_reset(void)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  PrintAPI("perform_SW_reset")

  CS_Reset_t rst_type = CS_RESET_FACTORY_RESET;

  if (DATAPACK_writeStruct(&cmd_buf[0], CSMT_RESET, sizeof(CS_Reset_t), (void *)&rst_type) == DATAPACK_OK)
  {
    err = AT_sendcmd(_Adapter_Handle, SID_CS_RESET, &cmd_buf[0], &rsp_buf[0]);
    if (err == ATSTATUS_OK)
    {
      PrintDBG("<Cellular_Service> Factory device reset done")
      retval = CELLULAR_OK;
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PrintErr("<Cellular_Service> error during Factory device reset")
  }
  return (retval);
}

static void CELLULAR_reset_context(void)
{
  /* init cs_ctxt_urc_subscription */
  cs_ctxt_urc_subscription.eps_network_registration = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.gprs_network_registration = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.cs_network_registration = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.eps_location_info = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.gprs_location_info = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.cs_location_info = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.signal_quality = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.packet_domain_event = CELLULAR_FALSE;
  cs_ctxt_urc_subscription.ping_rsp = CELLULAR_FALSE;

  /* init cs_ctxt_eps_location_info */
  cs_ctxt_eps_location_info.ci = 0U;
  cs_ctxt_eps_location_info.lac = 0U;
  cs_ctxt_eps_location_info.ci_updated = CELLULAR_FALSE;
  cs_ctxt_eps_location_info.lac_updated = CELLULAR_FALSE;

  /* init cs_ctxt_gprs_location_info */
  cs_ctxt_gprs_location_info.ci = 0U;
  cs_ctxt_gprs_location_info.lac = 0U;
  cs_ctxt_gprs_location_info.ci_updated = CELLULAR_FALSE;
  cs_ctxt_gprs_location_info.lac_updated = CELLULAR_FALSE;

  /* init cs_ctxt_cs_location_info */
  cs_ctxt_cs_location_info.ci = 0U;
  cs_ctxt_cs_location_info.lac = 0U;
  cs_ctxt_cs_location_info.ci_updated = CELLULAR_FALSE;
  cs_ctxt_cs_location_info.lac_updated = CELLULAR_FALSE;

  /* init network states */
  cs_ctxt_eps_network_reg_state = CS_NRS_UNKNOWN;
  cs_ctxt_gprs_network_reg_state = CS_NRS_UNKNOWN;
  cs_ctxt_cs_network_reg_state = CS_NRS_UNKNOWN;
}

static void CELLULAR_reset_socket_context(void)
{
  PrintDBG("CELLULAR_reset_socket_context")
  uint8_t cpt;
  for (cpt = 0U; cpt < CELLULAR_MAX_SOCKETS; cpt ++)
  {
    socket_init((socket_handle_t)cpt);
  }
}

static CS_Status_t CELLULAR_init(void)
{
  register_event_callback = NULL;

  if (SysCtrl_getDeviceDescriptor(DEVTYPE_MODEM_CELLULAR, &modem_device_infos) != SCSTATUS_OK)
  {
    return (CELLULAR_ERROR);
  }

  /* init ATCore & IPC layers*/
  AT_init();
  _Adapter_Handle = AT_open(&modem_device_infos, CELLULAR_idle_event_notif, CELLULAR_urc_notif);

  /* init local context variables */
  CELLULAR_reset_context();

  /* init socket context */
  CELLULAR_reset_socket_context();

  return (CELLULAR_OK);
}

static void CELLULAR_idle_event_notif(void)
{
  PrintAPI("<Cellular_Service> idle event notif")
  eventReceived++;

  if (register_event_callback != NULL)
  {
    /* inform client that an event has been received */
    (* register_event_callback)(eventReceived);
  }
}

static void CELLULAR_urc_notif(at_buf_t *p_rsp_buf)
{
  csint_msgType_t msgtype;

  PrintAPI("<Cellular_Service> CELLULAR_urc_notif")

  msgtype = (csint_msgType_t) DATAPACK_readMsgType(p_rsp_buf);

  /* --- EPS NETWORK REGISTRATION URC --- */
  if ((msgtype == CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS) &&
      (cs_ctxt_urc_subscription.eps_network_registration == CELLULAR_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS,
                            sizeof(CS_NetworkRegState_t), (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_eps_network_reg_state)
      {
        PrintDBG("<Cellular_Service> EPS network registration updated: %d", rx_state)
        cs_ctxt_eps_network_reg_state = rx_state;
        if (urc_eps_network_registration_callback != NULL)
        {
          /* TODO pack datas to client ?? */
          (* urc_eps_network_registration_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> EPS network registration unchanged")
      }
    }
  }
  /* --- GPRS NETWORK REGISTRATION URC --- */
  else if ((msgtype == CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS) &&
           (cs_ctxt_urc_subscription.gprs_network_registration == CELLULAR_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS,
                            sizeof(CS_NetworkRegState_t), (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_gprs_network_reg_state)
      {
        PrintDBG("<Cellular_Service> GPRS network registration updated: %d", rx_state)
        cs_ctxt_gprs_network_reg_state = rx_state;
        if (urc_gprs_network_registration_callback != NULL)
        {
          /* TODO pack datas to client ?? */
          (* urc_gprs_network_registration_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> GPRS network registration unchanged")
      }
    }
  }
  /* --- CS NETWORK REGISTRATION URC --- */
  else if ((msgtype == CSMT_URC_CS_NETWORK_REGISTRATION_STATUS) &&
           (cs_ctxt_urc_subscription.cs_network_registration == CELLULAR_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_CS_NETWORK_REGISTRATION_STATUS,
                            sizeof(CS_NetworkRegState_t), (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_cs_network_reg_state)
      {
        PrintDBG("<Cellular_Service> CS network registration updated: %d", rx_state)
        cs_ctxt_cs_network_reg_state = rx_state;
        if (urc_cs_network_registration_callback != NULL)
        {
          /* TODO pack datas to client ?? */
          (* urc_cs_network_registration_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> CS network registration unchanged")
      }
    }
  }
  /* --- EPS LOCATION INFORMATION URC --- */
  else if ((msgtype == CSMT_URC_EPS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.eps_location_info == CELLULAR_TRUE))
  {
    CS_Bool_t loc_update = CELLULAR_FALSE;
    csint_location_info_t rx_loc;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_EPS_LOCATION_INFO,
                            sizeof(csint_location_info_t), (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CELLULAR_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_eps_location_info.ci)
        {
          /* ci has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_eps_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_eps_location_info.ci_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_eps_location_info.ci_updated = CELLULAR_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CELLULAR_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_eps_location_info.lac)
        {
          /* lac has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_eps_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_eps_location_info.lac_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_eps_location_info.lac_updated = CELLULAR_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CELLULAR_TRUE)
      {
        if (urc_eps_location_info_callback != NULL)
        {
          PrintDBG("<Cellular_Service> EPS location information info updated: lac=%d, ci=%d", rx_loc.lac, rx_loc.ci)
          /* TODO pack datas to client ?? */
          (* urc_eps_location_info_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> EPS location information unchanged")
      }
    }
  }
  /* --- GPRS LOCATION INFORMATION URC --- */
  else if ((msgtype == CSMT_URC_GPRS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.gprs_location_info == CELLULAR_TRUE))
  {
    CS_Bool_t loc_update = CELLULAR_FALSE;
    csint_location_info_t rx_loc;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_GPRS_LOCATION_INFO,
                            sizeof(csint_location_info_t), (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CELLULAR_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_gprs_location_info.ci)
        {
          /* ci has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_gprs_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_gprs_location_info.ci_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_gprs_location_info.ci_updated = CELLULAR_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CELLULAR_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_gprs_location_info.lac)
        {
          /* lac has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_gprs_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_gprs_location_info.lac_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_gprs_location_info.lac_updated = CELLULAR_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CELLULAR_TRUE)
      {
        if (urc_gprs_location_info_callback != NULL)
        {
          PrintDBG("<Cellular_Service> GPRS location information info updated: lac=%d, ci=%d", rx_loc.lac, rx_loc.ci)
          /* TODO pack datas to client ?? */
          (* urc_gprs_location_info_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> GPRS location information unchanged")
      }
    }
  }
  /* --- CS LOCATION INFORMATION URC --- */
  else if ((msgtype == CSMT_URC_CS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.cs_location_info == CELLULAR_TRUE))
  {
    CS_Bool_t loc_update = CELLULAR_FALSE;
    csint_location_info_t rx_loc;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_CS_LOCATION_INFO,
                            sizeof(csint_location_info_t), (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CELLULAR_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_cs_location_info.ci)
        {
          /* ci has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_cs_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_cs_location_info.ci_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_cs_location_info.ci_updated = CELLULAR_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CELLULAR_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_cs_location_info.lac)
        {
          /* lac has change */
          loc_update = CELLULAR_TRUE;
          cs_ctxt_cs_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_cs_location_info.lac_updated == CELLULAR_FALSE)
        {
          loc_update = CELLULAR_TRUE;
          cs_ctxt_cs_location_info.lac_updated = CELLULAR_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CELLULAR_TRUE)
      {
        if (urc_cs_location_info_callback != NULL)
        {
          PrintDBG("<Cellular_Service> CS location information info updated: lac=%d, ci=%d", rx_loc.lac, rx_loc.ci)
          /* TODO pack datas to client ?? */
          (* urc_cs_location_info_callback)();
        }
      }
      else
      {
        PrintDBG("<Cellular_Service> CS location information unchanged")
      }
    }
  }
  /* --- SIGNAL QUALITY URC --- */
  else if ((msgtype == CSMT_URC_SIGNAL_QUALITY) &&
           (cs_ctxt_urc_subscription.signal_quality == CELLULAR_TRUE))
  {
    CS_SignalQuality_t local_sig_qual;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_SIGNAL_QUALITY,
                            sizeof(CS_SignalQuality_t), (void *)&local_sig_qual) == DATAPACK_OK)
    {

      if (urc_signal_quality_callback != NULL)
      {
        PrintINFO("<Cellular_Service> CS signal quality info updated: rssi=%d, ber=%d", local_sig_qual.rssi, local_sig_qual.ber)
        /* TODO pack datas to client ?? */
        (* urc_signal_quality_callback)();
      }
    }
  }
  /* --- SOCKET DATA PENDING URC --- */
  else if (msgtype == CSMT_URC_SOCKET_DATA_PENDING)
  {
    /* unpack datas received */
    socket_handle_t sockHandle;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_SOCKET_DATA_PENDING,
                            sizeof(socket_handle_t), (void *)&sockHandle) == DATAPACK_OK)
    {
      if (sockHandle != CS_INVALID_SOCKET_HANDLE)
      {
        /* inform client that data are pending */
        if (cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback != NULL)
        {
          (* cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback)(sockHandle);
        }
      }
    }
  }
  /* --- SOCKET DATA CLOSED BY REMOTE URC --- */
  else if (msgtype == CSMT_URC_SOCKET_CLOSED)
  {
    /* unpack datas received */
    socket_handle_t sockHandle;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_SOCKET_CLOSED,
                            sizeof(socket_handle_t), (void *)&sockHandle) == DATAPACK_OK)
    {
      if (sockHandle != CS_INVALID_SOCKET_HANDLE)
      {
        /* inform client that socket has been closed by remote  */
        if (cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback != NULL)
        {
          (* cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback)(sockHandle);
        }

        /* do not deallocate socket handle and reinit socket parameters
        *   socket_deallocateHandle(sockHandle);
        *   client has to confirm with a call to CDS_socket_close()
        */
      }
    }
  }
  /* --- PACKET DOMAIN EVENT URC --- */
  else if ((msgtype == CSMT_URC_PACKET_DOMAIN_EVENT) &&
           (cs_ctxt_urc_subscription.packet_domain_event == CELLULAR_TRUE))
  {
    /* unpack datas received */
    csint_PDN_event_desc_t pdn_event;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_PACKET_DOMAIN_EVENT,
                            sizeof(csint_PDN_event_desc_t), (void *)&pdn_event) == DATAPACK_OK)
    {
      PrintDBG("PDN event: origine=%d scope=%d type=%d (user cid=%d) ",
               pdn_event.event_origine, pdn_event.event_scope,
               pdn_event.event_type, pdn_event.conf_id)

      /* is it a valid PDN ? */
      if ((pdn_event.conf_id == CS_PDN_USER_CONFIG_1) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_2) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_3) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_4) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_5) ||
          (pdn_event.conf_id == CS_PDN_PREDEF_CONFIG))
      {
        if (urc_packet_domain_event_callback[pdn_event.conf_id] != NULL)
        {
          CS_PDN_event_t conv_pdn_event = convert_to_PDN_event(pdn_event);
          (* urc_packet_domain_event_callback[pdn_event.conf_id])(pdn_event.conf_id, conv_pdn_event);
        }
      }
      else if (pdn_event.conf_id == CS_PDN_ALL)
      {
        CS_PDN_event_t conv_pdn_event = convert_to_PDN_event(pdn_event);

        /* event concerns all registered PDN */
        for (uint8_t loop = 0U; loop <= CS_PDN_CONFIG_MAX; loop++)
        {
          if (urc_packet_domain_event_callback[loop] != NULL)
          {
            (* urc_packet_domain_event_callback[loop])((CS_PDN_conf_id_t)loop, conv_pdn_event);
          }
        }
      }
      else
      {
        PrintINFO("PDN not identified")
      }
    }
  }
  /* --- PING URC --- */
  else if (msgtype == CSMT_URC_PING_RSP)
  {
    /* unpack datas received */
    CS_Ping_response_t ping_rsp;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_PING_RSP,
                            sizeof(CS_Ping_response_t), (void *)&ping_rsp) == DATAPACK_OK)
    {
      PrintDBG("ping URC received at CS level")
      if (urc_ping_rsp_callback != NULL)
      {
        (* urc_ping_rsp_callback)(ping_rsp);
      }
    }
  }
  /* --- MODEM EVENT URC --- */
  else if (msgtype == CSMT_URC_MODEM_EVENT)
  {
    /* unpack datas received */
    CS_ModemEvent_t modem_events;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_URC_MODEM_EVENT,
                            sizeof(CS_ModemEvent_t), (void *)&modem_events) == DATAPACK_OK)
    {
      PrintDBG("MODEM events received= 0x%x", modem_events)
      if (urc_modem_event_callback != NULL)
      {
        (* urc_modem_event_callback)(modem_events);
      }
    }
  }
  else
  {
    PrintDBG("ignore received URC (type=%d)", msgtype)
  }
}

static CS_Status_t CELLULAR_analyze_error_report(at_buf_t *p_rsp_buf)
{
  CS_Status_t retval;
  csint_msgType_t msgtype;

  PrintAPI("<Cellular_Service> CELLULAR_analyze_error_report")
  retval = CELLULAR_ERROR;
  msgtype = (csint_msgType_t) DATAPACK_readMsgType(p_rsp_buf);

  /* check if we have received an error report */
  if (msgtype == CSMT_ERROR_REPORT)
  {
    csint_error_report_t error_report;
    if (DATAPACK_readStruct(p_rsp_buf, CSMT_ERROR_REPORT,
                            sizeof(csint_error_report_t),
                            (void *)&error_report) == DATAPACK_OK)
    {
      switch (error_report.error_type)
      {
        /* SIM error */
        case CSERR_SIM:
          retval = convert_SIM_error(&error_report);
          break;

        /* default error */
        default:
          retval = CELLULAR_ERROR;
          break;
      }
    }
  }

  PrintDBG("CS returned modified value after error report analysis = %d", retval)
  return (retval);
}

static CS_Status_t convert_SIM_error(csint_error_report_t *p_error_report)
{
  CS_Status_t retval;

  /* convert SIM state to Cellular Service error returned to the client */
  switch (p_error_report->sim_state)
  {
    case CS_SIMSTATE_SIM_NOT_INSERTED:
      retval = CELLULAR_SIM_NOT_INSERTED;
      break;
    case CS_SIMSTATE_SIM_BUSY:
      retval = CELLULAR_SIM_BUSY;
      break;
    case CS_SIMSTATE_SIM_WRONG:
    case CS_SIMSTATE_SIM_FAILURE:
      retval = CELLULAR_SIM_ERROR;
      break;
    case CS_SIMSTATE_SIM_PIN_REQUIRED:
    case CS_SIMSTATE_SIM_PIN2_REQUIRED:
    case CS_SIMSTATE_SIM_PUK_REQUIRED:
    case CS_SIMSTATE_SIM_PUK2_REQUIRED:
      retval = CELLULAR_SIM_PIN_OR_PUK_LOCKED;
      break;
    case CS_SIMSTATE_INCORRECT_PASSWORD:
      retval = CELLULAR_SIM_INCORRECT_PASSWORD;
      break;
    default:
      retval = CELLULAR_SIM_ERROR;
      break;
  }
  return (retval);
}

static void modem_reset_update_socket_state(void)
{
  /* When a Modem RESET is requested, socket state will be update as follow:
  *
  * - if SOCKETSTATE_NOT_ALLOC => no modification
  * - if SOCKETSTATE_CREATED => keep socket context, still valid from user (OPEN not requested to modem yet)
  * - if SOCKETSTATE_CONNECTED => socket is lost, invalid state
  * - if SOCKETSTATE_ALLOC_BUT_INVALID => socket already in an invalid state
  */
  uint8_t cpt;

  for (cpt = 0U; cpt < CELLULAR_MAX_SOCKETS; cpt++)
  {
    switch (cs_ctxt_sockets_info[cpt].state)
    {
      case SOCKETSTATE_NOT_ALLOC:
      case SOCKETSTATE_ALLOC_BUT_INVALID:
        /* nothing to do */
        break;

      case SOCKETSTATE_CREATED:
        /* nothing to do - keep user context */
        break;

      case SOCKETSTATE_CONNECTED:
        /* modem reset occured, context no more valid
        *  waiting for a close from client
        */
        cs_ctxt_sockets_info[cpt].state = SOCKETSTATE_ALLOC_BUT_INVALID;
        break;

      default:
        PrintErr("unknown socket state, Should not happen")
        break;
    }
  }
}

static void socket_init(socket_handle_t index)
{
  PrintAPI("<Cellular_Service> SOCKET_init (index=%d)", index)

  cs_ctxt_sockets_info[index].socket_handle = index;
  cs_ctxt_sockets_info[index].state = SOCKETSTATE_NOT_ALLOC;
  cs_ctxt_sockets_info[index].config = CS_SON_NO_OPTION;

  cs_ctxt_sockets_info[index].addr_type = CS_IPAT_IPV4;
  cs_ctxt_sockets_info[index].protocol = CS_TCP_PROTOCOL;
  cs_ctxt_sockets_info[index].local_port = 0U;
  cs_ctxt_sockets_info[index].conf_id = CS_PDN_NOT_DEFINED;

  cs_ctxt_sockets_info[index].ip_max_packet_size = DEFAULT_IP_MAX_PACKET_SIZE;
  cs_ctxt_sockets_info[index].trp_max_timeout = DEFAULT_TRP_MAX_TIMEOUT;
  cs_ctxt_sockets_info[index].trp_conn_setup_timeout = DEFAULT_TRP_CONN_SETUP_TIMEOUT;
  cs_ctxt_sockets_info[index].trp_transfer_timeout = DEFAULT_TRP_TRANSFER_TIMEOUT;
  cs_ctxt_sockets_info[index].trp_connect_mode = CS_CM_COMMAND_MODE;
  cs_ctxt_sockets_info[index].trp_suspend_timeout = DEFAULT_TRP_SUSPEND_TIMEOUT;
  cs_ctxt_sockets_info[index].trp_rx_timeout = DEFAULT_TRP_RX_TIMEOUT;

  /* socket callback functions pointers */
  cs_ctxt_sockets_info[index].socket_data_ready_callback = NULL;
  cs_ctxt_sockets_info[index].socket_data_sent_callback = NULL;
  cs_ctxt_sockets_info[index].socket_remote_close_callback = NULL;
}

static socket_handle_t socket_allocateHandle(void)
{
  socket_handle_t socket_handle = CS_INVALID_SOCKET_HANDLE;
  uint8_t cpt;

  for (cpt = 0U; cpt < CELLULAR_MAX_SOCKETS; cpt++)
  {
    if (cs_ctxt_sockets_info[cpt].state == SOCKETSTATE_NOT_ALLOC)
    {
      /* free socket handle found */
      socket_handle = (socket_handle_t)cpt;
      break;
    }
  }

  return (socket_handle);
}

static void socket_deallocateHandle(socket_handle_t sockhandle)
{
  PrintINFO("socket_deallocateHandle %d", sockhandle)
  socket_init(sockhandle);
}

static CS_Status_t socket_create(socket_handle_t sockhandle,
                                 CS_IPaddrType_t addr_type,
                                 CS_TransportProtocol_t protocol,
                                 uint16_t local_port,
                                 CS_PDN_conf_id_t cid)
{
  CS_Status_t retval;

  cs_ctxt_sockets_info[sockhandle].addr_type = addr_type;
  cs_ctxt_sockets_info[sockhandle].protocol = protocol;
  cs_ctxt_sockets_info[sockhandle].local_port = local_port;
  cs_ctxt_sockets_info[sockhandle].conf_id = cid;

  if (cs_ctxt_sockets_info[sockhandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    /* update socket state */
    cs_ctxt_sockets_info[sockhandle].state = SOCKETSTATE_CREATED;
    retval = CELLULAR_OK;
  }
  else
  {
    PrintErr("<Cellular_Service> socket handle %d not available", sockhandle)
    retval = CELLULAR_ERROR;
  }

  return (retval);
}

static CS_Status_t socket_bind(socket_handle_t sockhandle,
                               uint16_t local_port)
{
  CS_Status_t retval;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockhandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PrintErr("<Cellular_Service> invalid socket handle %d (bind)", sockhandle)
    retval = CELLULAR_ERROR;
  }
  else
  {
    /* set the local port */
    cs_ctxt_sockets_info[sockhandle].local_port = local_port;
    retval = CELLULAR_OK;
  }

  return (retval);
}

static CS_Status_t socket_configure(socket_handle_t sockhandle,
                                    CS_SocketOptionLevel_t opt_level,
                                    CS_SocketOptionName_t opt_name,
                                    void *p_opt_val)
{
  CS_Status_t retval = CELLULAR_OK;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockhandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PrintErr("<Cellular_Service> invalid socket handle %d (cfg)", sockhandle)
    retval = CELLULAR_ERROR;
  }

  if (p_opt_val == NULL)
  {
    PrintErr("<Cellular_Service> NULL ptr")
    retval = CELLULAR_ERROR;
  }

  if (retval == CELLULAR_OK)
  {
    uint16_t *p_uint16;
    uint8_t  *p_uint8;
    CS_ConnectionMode_t *p_CS_CM_t;

    if (opt_level == CS_SOL_IP)
    {
      switch (opt_name)
      {
        case CS_SON_IP_MAX_PACKET_SIZE:
          p_uint16 = p_opt_val;
          if (*p_uint16 <= DEFAULT_IP_MAX_PACKET_SIZE)
          {
            if (*p_uint16 == 0U)
            {
              cs_ctxt_sockets_info[sockhandle].ip_max_packet_size = DEFAULT_IP_MAX_PACKET_SIZE;
            }
            else
            {
              cs_ctxt_sockets_info[sockhandle].ip_max_packet_size = *p_uint16;
            }
            PrintDBG("DBG: trp_conn_setup_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_conn_setup_timeout)
          }
          else
          {
            PrintErr("<Cellular_Service> max_packet_size value out of range ")
            retval = CELLULAR_ERROR;
          }
          break;

        default:
          PrintErr("<Cellular_Service> invalid option name for IP")
          retval = CELLULAR_ERROR;
          break;
      }
    }
    else if (opt_level == CS_SOL_TRANSPORT)
    {
      switch (opt_name)
      {
        case CS_SON_TRP_MAX_TIMEOUT:
          p_uint16 = p_opt_val;
          cs_ctxt_sockets_info[sockhandle].trp_max_timeout = *p_uint16;
          PrintDBG("DBG: trp_max_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_max_timeout)
          break;

        case CS_SON_TRP_CONNECT_SETUP_TIMEOUT:
          p_uint16 = p_opt_val;
          cs_ctxt_sockets_info[sockhandle].trp_conn_setup_timeout = *p_uint16;
          PrintDBG("DBG: trp_conn_setup_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_conn_setup_timeout)
          break;

        case CS_SON_TRP_TRANSFER_TIMEOUT:
          p_uint8 = p_opt_val;
          cs_ctxt_sockets_info[sockhandle].trp_transfer_timeout = *p_uint8;
          PrintDBG("DBG: trp_transfer_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_transfer_timeout)
          break;

        case CS_SON_TRP_CONNECT_MODE:
          p_CS_CM_t = p_opt_val;
          if (*p_CS_CM_t == CS_CM_COMMAND_MODE)
          {
            cs_ctxt_sockets_info[sockhandle].trp_connect_mode = *p_CS_CM_t;
            PrintDBG("DBG: trp_connect_mode = %d", cs_ctxt_sockets_info[sockhandle].trp_connect_mode)
          }
          else
          {
            PrintErr("<Cellular_Service> Connection mode not supported")
            retval = CELLULAR_ERROR;
          }
          break;

        case CS_SON_TRP_SUSPEND_TIMEOUT:
          p_uint16 = p_opt_val;
          cs_ctxt_sockets_info[sockhandle].trp_suspend_timeout =  *p_uint16;
          PrintDBG("DBG: trp_suspend_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_suspend_timeout)
          break;

        case CS_SON_TRP_RX_TIMEOUT:
          p_uint8 = p_opt_val;
          cs_ctxt_sockets_info[sockhandle].trp_rx_timeout = *p_uint8;
          PrintDBG("DBG: trp_rx_timeout = %d", cs_ctxt_sockets_info[sockhandle].trp_rx_timeout)
          break;

        default:
          PrintErr("<Cellular_Service> invalid option name for TRANSPORT")
          retval = CELLULAR_ERROR;
          break;
      }
    }
    else
    {
      PrintErr("<Cellular_Service> invalid socket option level ")
      retval = CELLULAR_ERROR;
    }
  }

  if (retval == CELLULAR_OK)
  {
    /* set a flag to indicate that this parameter has been set */
    cs_ctxt_sockets_info[sockhandle].config |= opt_name;
  }

  return (retval);
}

static CS_Status_t socket_configure_remote(socket_handle_t sockhandle,
                                           CS_IPaddrType_t ip_addr_type,
                                           CS_CHAR_t *p_ip_addr_value,
                                           uint16_t remote_port)
{
  CS_Status_t retval = CELLULAR_OK;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockhandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PrintErr("<Cellular_Service> invalid socket handle %d (cfg remote)", sockhandle)
    return (CELLULAR_ERROR);
  }

  if (p_ip_addr_value == NULL)
  {
    PrintErr("<Cellular_Service> NULL ptr")
    return (CELLULAR_ERROR);
  }

  cs_ctxt_sockets_info[sockhandle].remote_port = remote_port;
  cs_ctxt_sockets_info[sockhandle].ip_addr_type = ip_addr_type;
  memset((void *) &cs_ctxt_sockets_info[sockhandle].ip_addr_value, 0, MAX_IP_ADDR_SIZE);
  memcpy((void *) &cs_ctxt_sockets_info[sockhandle].ip_addr_value, (void *)p_ip_addr_value, strlen((const char *)p_ip_addr_value));

  PrintDBG("DBG: remote_port=%d", cs_ctxt_sockets_info[sockhandle].remote_port)
  PrintDBG("DBG: ip_addr_type=%d", cs_ctxt_sockets_info[sockhandle].ip_addr_type)
  PrintDBG("DBG: ip_addr cb=%s", cs_ctxt_sockets_info[sockhandle].ip_addr_value)

  return (retval);
}

static CS_PDN_event_t convert_to_PDN_event(csint_PDN_event_desc_t event_desc)
{
  CS_PDN_event_t ret = CS_PDN_EVENT_OTHER;

  if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
      (event_desc.event_scope == CGEV_EVENT_SCOPE_GLOBAL) &&
      (event_desc.event_type == CGEV_EVENT_TYPE_DETACH))
  {
    ret = CS_PDN_EVENT_NW_DETACH;
  }
  else if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
           (event_desc.event_scope == CGEV_EVENT_SCOPE_GLOBAL) &&
           (event_desc.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
  {
    ret = CS_PDN_EVENT_NW_DEACT;
  }
  else if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
           (event_desc.event_scope == CGEV_EVENT_SCOPE_PDN) &&
           (event_desc.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
  {
    ret = CS_PDN_EVENT_NW_PDN_DEACT;
  }
  else
  {
    /* ignored */
  }

  return (ret);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


