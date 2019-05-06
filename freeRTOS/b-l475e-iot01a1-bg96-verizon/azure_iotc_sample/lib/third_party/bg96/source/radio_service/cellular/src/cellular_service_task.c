/**
  ******************************************************************************
  * @file    cellular_service_task.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Task
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
#include "dc_common.h"

#include "string.h"
#include "cellular_init.h"
#include "cellular_service.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_task.h"
#include "cmsis_os.h"
#include "error_handler.h"
#include "plf_config.h"
#include "gpio.h"
#include "trace_interface.h"

#if (!USE_DEFAULT_SETUP == 1)
#include "menu_utils.h"
#include "setup.h"
#include "app_select.h"
#endif

/* Private defines -----------------------------------------------------------*/
#define CST_SETUP_NFMC       (1)

/* Test FOTA FEATURE */
#define CST_FOTA_TEST        (0)

/* Test NFMC activation */
#define CST_TEST_NFMC        (0)
#if (CST_TEST_NFMC == 1)
static uint32_t CST_nfmc_test_count = 0;
#define CST_NFMC_TEST_COUNT_MAX 9
#endif

#define CST_TEST_REGISTER_FAIL        (0)
#if (CST_TEST_REGISTER_FAIL == 1)
static uint32_t CST_register_fail_test_count = 0;
#define CST_REGISTER_FAIL_TEST_COUNT_MAX 2
#endif

#define CST_MODEM_POWER_ON_LABEL "Modem power on"

#define CST_COUNT_FAIL_MAX (5U)

#define GOOD_PINCODE "" /* SET PIN CODE HERE (for exple "1234"), if no PIN code, use an string empty "" */
#define CST_MODEM_POLLING_PERIOD_DEFAULT 5000U

#define CST_BAD_SIG_RSSI 99U

#define CST_POWER_ON_RESET_MAX      5U
#define CST_RESET_MAX               5U
#define CST_INIT_MODEM_RESET_MAX    5U
#define CST_CSQ_MODEM_RESET_MAX     5U
#define CST_GNS_MODEM_RESET_MAX     5U
#define CST_ATTACH_RESET_MAX        5U
#define CST_DEFINE_PDN_RESET_MAX    5U
#define CST_ACTIVATE_PDN_RESET_MAX  5U
#define CST_CELLULAR_DATA_RETRY_MAX 5U
#define CST_GLOBAL_RETRY_MAX        5U

#define CST_LABEL "Cellular Service"
#define CST_VERSION_APPLI     (uint16_t)2

#define CST_INPUT_STRING_SIZE    20
#define CST_DISPLAY_STRING_SIZE  20
#define CST_APN_SIZE             20

#define CST_DEFAULT_PARAMA_NB    12U
#define CST_NFMC_BOOL_SIZE        2
#define CST_NFMC_VALUES_SIZE      6
#define CST_NFMC_TEMPO_NB         7U

#define CST_PDN_ACTIVATE_RETRY_DELAY 30000U
#define CST_NETWORK_STATUS_DELAY     25000U

#define CST_FOTA_TIMEOUT      (360000U) /* 6 min (calibrated for cat-M1 network, increase it for cat-NB1) */


#define CST_SIM_POLL_COUNT     200U    /* 20s */

#define CST_RESET_DEBUG (0)

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#define PrintCellularService(format, args...)      TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, format, ## args)
#define PrintCellularServiceErr(format, args...)   TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_ERR, "ERROR " format, ## args)
#else
#define PrintCellularService(format, args...)      printf(format , ## args);
#define PrintCellularServiceErr(format, args...)   printf(format , ## args);
#endif /* USE_PRINTF */
#else
#define PrintCellularService(format, args...)   do {} while(0);
#define PrintCellularServiceErr(format, args...)  do {} while(0);
#endif /* USE_TRACE_CELLULAR_SERVICE */

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  CST_MESSAGE_CS_EVENT    = 0,
  CST_MESSAGE_DC_EVENT    = 1,
  CST_MESSAGE_URC_EVENT   = 2,
  CST_MESSAGE_TIMER_EVENT = 3,
  CST_MESSAGE_CMD         = 4
} CST_message_type_t;

typedef enum
{
  CST_OFF_EVENT,
  CST_INIT_EVENT,
  CST_MODEM_POWER_ON_EVENT,
  CST_MODEM_POWERED_ON_EVENT,
  CST_MODEM_INITIALIZED_EVENT,
  CST_NETWORK_CALLBACK_EVENT,
  CST_SIGNAL_QUALITY_EVENT,
  CST_NW_REG_TIMEOUT_TIMER_EVENT,
  CST_NETWORK_STATUS_EVENT,
  CST_NETWORK_STATUS_OK_EVENT,
  CST_MODEM_ATTACHED_EVENT,
  CST_PDP_ACTIVATED_EVENT,
  CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT,
  CST_PDN_STATUS_EVENT,
  CST_CELLULAR_DATA_FAIL_EVENT,
  CST_FAIL_EVENT,
  CST_POLLING_TIMER,
  CST_MODEM_URC,
  CST_NO_EVENT,
  CST_CMD_UNKWONW_EVENT
} CST_autom_event_t;

typedef enum
{
  CST_NO_FAIL,
  CST_MODEM_POWER_ON_FAIL,
  CST_MODEM_RESET_FAIL,
  CST_MODEM_CSQ_FAIL,
  CST_MODEM_GNS_FAIL,
  CST_MODEM_REGISTER_FAIL,
  CST_MODEM_ATTACH_FAIL,
  CST_MODEM_PDP_DEFINE_FAIL,
  CST_MODEM_PDP_ACTIVATION_FAIL,
  CST_MODEL_CELLULAR_DATA_FAIL
} CST_fail_cause_t;

typedef enum
{
  CST_SIM_SLOT_MODEM_SOCKET         = 0,
  CST_SIM_SLOT_MODEM_EMBEDDED_SIM   = 1,
  CST_SIM_SLOT_STM32_EMBEDDED_SIM   = 2
} CST_sim_slot_type_t;

typedef struct
{
  uint16_t  type ;
  uint16_t  id  ;
} CST_message_t;

typedef struct
{
  CST_state_t          current_state;
  CST_fail_cause_t     fail_cause;
  CS_PDN_event_t       pdn_status;
  CS_SignalQuality_t   signal_quality;
  CS_NetworkRegState_t current_EPS_NetworkRegState;
  CS_NetworkRegState_t current_GPRS_NetworkRegState;
  CS_NetworkRegState_t current_CS_NetworkRegState;
  CS_Status_t          sim_mode;
  uint16_t             set_pdn_mode;
  uint16_t             activate_pdn_nfmc_tempo_count;
  uint16_t             register_retry_tempo_count;
  uint16_t             power_on_reset_count ;
  uint16_t             reset_reset_count ;
  uint16_t             init_modem_reset_count ;
  uint16_t             csq_reset_count ;
  uint16_t             gns_reset_count ;
  uint16_t             attach_reset_count ;
  uint16_t             activate_pdn_reset_count ;
  uint16_t             cellular_data_retry_count ;
  uint16_t             global_retry_count ;
} CST_context_t;

typedef struct
{
  uint32_t  active;
  uint32_t  value[CST_NFMC_TEMPO_NB];
  uint32_t  tempo[CST_NFMC_TEMPO_NB];
} CST_nfmc_context_t;

typedef struct
{
  uint8_t            *APN;
  CS_PDN_conf_id_t    CID;
  CST_sim_slot_type_t SIM_SLOT;
} CST_cellular_params_t;

#if (USE_DEFAULT_SETUP == 1)
typedef enum
{
  CST_PARAM_PDN_MODE   = 0,
  CST_PARAM_APN        = 1,
  CST_PARAM_CID        = 2,
  CST_PARAM_SIM_SLOT   = 3,
  CST_PARAM_NFMC       = 4,
  CST_PARAM_NFMC_TEMPO = 5,
} CST_setup_params_t;
#endif

/* Private variables ---------------------------------------------------------*/
static CST_cellular_params_t CST_cellular_params =
{
  NULL,
  (CS_PDN_conf_id_t)0,
  (CST_sim_slot_type_t)0
};

static uint8_t *CST_default_setup_table[CST_DEFAULT_PARAMA_NB] =
{
  CST_DEFAULT_MODE_STRING,
  CST_DEFAULT_APN_STRING,
  CST_DEFAULT_CID_STRING,
  CST_DEFAULT_SIM_SLOT_STRING,
  CST_DEFAULT_NFMC_ACTIVATION_STRING,
  CST_DEFAULT_NFMC_TEMPO1_STRING,
  CST_DEFAULT_NFMC_TEMPO2_STRING,
  CST_DEFAULT_NFMC_TEMPO3_STRING,
  CST_DEFAULT_NFMC_TEMPO4_STRING,
  CST_DEFAULT_NFMC_TEMPO5_STRING,
  CST_DEFAULT_NFMC_TEMPO6_STRING,
  CST_DEFAULT_NFMC_TEMPO7_STRING
};

static osMessageQId      cst_queue_id;
static osTimerId         cst_polling_timer_handle;
static osTimerId         CST_pdn_activate_retry_timer_handle;
static osTimerId         CST_network_status_timer_handle;
static osTimerId         CST_register_retry_timer_handle;
static osTimerId         CST_fota_timer_handle;
static uint64_t          CST_IMSI;
static dc_cellular_info_t cst_cellular_info;
static dc_sim_info_t      cst_sim_info;

#if (!USE_DEFAULT_SETUP == 1)
static uint8_t CST_input_string[CST_INPUT_STRING_SIZE];
static uint8_t  CST_APN_string[CST_APN_SIZE];

#if (CST_SETUP_NFMC == 1)
static uint8_t CST_display_string[CST_DISPLAY_STRING_SIZE];
#endif /* (CST_SETUP_NFMC == 1) */

#endif /* (!USE_DEFAULT_SETUP == 1) */

static CST_nfmc_context_t CST_nfmc_context;
static CST_context_t     cst_context = {    CST_MODEM_INIT_STATE, CST_NO_FAIL, CS_PDN_EVENT_NW_DETACH,    /* Automaton State, FAIL Cause,  */
  { 0, 0},                              /* signal quality */
  CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING,
  CELLULAR_OK,                           /* sim_mode */
  0,                                     /* set_pdn_mode */
  0,                                     /* activate_pdn_nfmc_tempo_count */
  0,                                     /* register_retry_tempo_count */
  0, 0, 0, 0, 0, 0, 0, 0, 0              /* Reset counters */
};

uint8_t CST_signal_quality = 0U;

static CS_OperatorSelector_t    ctxt_operator =
{
  .mode = CS_NRM_AUTO,
  .format = CS_ONF_NOT_PRESENT,
  .operator_name = "00101",
};

static uint8_t  CST_polling_timer_flag = 0U;
static uint8_t CST_csq_count_fail      = 0U;

/* Global variables ----------------------------------------------------------*/
osThreadId CST_cellularServiceThreadId = NULL;

/* Private function prototypes -----------------------------------------------*/
static CS_Status_t  CST_send_message(uint16_t  type, uint16_t cmd);
static void CST_reset_fail_count(void);
static void CST_select_sim(CST_sim_slot_type_t sim_slot);
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event);
static void CST_network_reg_callback(void);
static void CST_modem_event_callback(CS_ModemEvent_t event);
static void CST_location_info_callback(void);
static void  CST_data_cache_set(dc_service_rt_state_t dc_service_state);
static void CST_location_info_callback(void);
static void CST_config_fail_mngt(char *msg_fail, CST_fail_cause_t fail_cause, uint16_t *fail_count, uint16_t fail_max);
static void CST_modem_init(void);
static CS_Status_t CST_set_signal_quality(void);
static CS_Status_t  CST_send_message(uint16_t  type, uint16_t cmd);
static void CST_get_device_all_infos(void);
static void CST_subscribe_all_net_events(void);
static void CST_subscribe_modem_events(void);
static void CST_notif_cb(dc_com_event_id_t dc_event_id, void *private_gui_data);
static CST_autom_event_t CST_get_autom_event(osEvent event);
static void CST_power_on_only_modem_mngt(void);
static void CST_power_on_modem_mngt(void);
static CS_Status_t CST_reset_modem(void);
static void CST_reset_modem_mngt(void);
static void CST_init_modem_mngt(void);
static void CST_net_register_mngt(void);
static void CST_signal_quality_test_mngt(void);
static void CST_network_status_test_mngt(void);
static void CST_network_event_mngt(void);
static void CST_attach_modem_mngt(void);
static void CST_modem_define_pdn(void);
static CS_Status_t CST_modem_activate_pdn(void);
static void CST_cellular_data_fail_mngt(void);
static void CST_pdn_event_mngt(void);
static void CST_init_state(CST_autom_event_t autom_event);
static void CST_reset_state(CST_autom_event_t autom_event);
static void CST_modem_on_state(CST_autom_event_t autom_event);
static void CST_modem_powered_on_state(CST_autom_event_t autom_event);
static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event);
static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event);
static void CST_network_status_ok_state(CST_autom_event_t autom_event);
static void CST_modem_registered_state(CST_autom_event_t autom_event);
static void CST_modem_pdn_activate_state(CST_autom_event_t autom_event);
static void CST_data_ready_state(CST_autom_event_t autom_event);
static void CST_fail_state(CST_autom_event_t autom_event);
static void CST_timer_handler(void);
static void CST_cellular_service_task(void const *argument);
static void CST_polling_timer_callback(void const *argument);
static void CST_pdn_activate_retry_timer_callback(void const *argument);
static void CST_network_status_timer_callback(void const *argument);
static void CST_register_retry_timer_callback(void const *argument);
static void CST_fota_timer_callback(void const *argument);
static uint64_t CST_util_ipow(uint32_t base, uint32_t exp);
static uint64_t CST_util_convertStringToInt64(uint8_t *p_string, uint32_t size);
static uint32_t CST_calculate_tempo_value(uint32_t nb, uint32_t value, uint64_t imsi);
static void CST_fill_nfmc_tempo(uint64_t imsi_p);
static void CST_modem_start(void);

#if (!USE_DEFAULT_SETUP == 1)
static void CST_setup_dump(void);
static void CST_setup_handler(void);
#else
static void CST_local_setup_handler(void);
#endif /* (!USE_DEFAULT_SETUP == 1) */


/* Private function Definition -----------------------------------------------*/

#if (!USE_DEFAULT_SETUP == 1)
static void CST_setup_handler(void)
{
  menu_utils_get_string((uint8_t *)"pdn config mode (0: Select CID alone / 1: Select APN and CID)", (uint8_t *)CST_input_string, sizeof(CST_input_string));
  if (CST_input_string[0] == '1')
  {
    cst_context.set_pdn_mode = 1U;
  }
  else
  {
    cst_context.set_pdn_mode = 0U;
  }

  menu_utils_get_string((uint8_t *)"APN ", (uint8_t *)CST_APN_string, sizeof(CST_APN_string));
  CST_cellular_params.APN = CST_APN_string;

  menu_utils_get_string((uint8_t *)"CID ", (uint8_t *)CST_input_string, sizeof(CST_input_string));
  CST_cellular_params.CID = (CS_PDN_conf_id_t)(CST_input_string[0] - '0');

  menu_utils_get_string((uint8_t *)"Sim Slot (0: socket / 1: embedded sim / 2: host) ", (uint8_t *)CST_input_string, sizeof(CST_input_string));
  CST_cellular_params.SIM_SLOT = (CST_sim_slot_type_t)(CST_input_string[0] - '0');


#if (CST_SETUP_NFMC == 1)
  menu_utils_get_string((uint8_t *)"NFMC activation (0: inactive / 1: active) ", (uint8_t *)CST_input_string, sizeof(CST_input_string));
  if (CST_input_string[0] == '1')
  {
    CST_nfmc_context.active = 1U;
  }
  else
  {
    CST_nfmc_context.active = 0U;
  }

  for (uint32_t i = 0U; i < CST_NFMC_TEMPO_NB ; i++)
  {
    sprintf((char *)CST_display_string, "NFMC value %ld  ", i + 1U);
    menu_utils_get_string(CST_display_string, (uint8_t *)CST_input_string, sizeof(CST_input_string));
    CST_nfmc_context.value[i] = (uint32_t)atoi((char *)CST_input_string);
  }
#else
  CST_nfmc_context.active = 1U;
  CST_nfmc_context.value[0] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO1_STRING);
  CST_nfmc_context.value[1] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO2_STRING);
  CST_nfmc_context.value[2] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO3_STRING);
  CST_nfmc_context.value[3] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO4_STRING);
  CST_nfmc_context.value[4] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO5_STRING);
  CST_nfmc_context.value[5] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO6_STRING);
  CST_nfmc_context.value[6] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO7_STRING);
#endif /* (CST_SETUP_NFMC == 1) */
}

static void CST_setup_dump(void)
{
  PrintCellularService("\n\r")
  if (cst_context.set_pdn_mode == 1U)
  {
    PrintCellularService("APN to add : %s\n\r",  CST_cellular_params.APN)
    PrintCellularService("CID to add : %d\n\r",  CST_cellular_params.CID)
  }
  else
  {
    PrintCellularService("CID used : %d\n\r",  CST_cellular_params.CID)
  }

  PrintCellularService("PDN activation : %d\n\r",  cst_context.set_pdn_mode)
  PrintCellularService("SIM_SLOT : %d\n\r",  CST_cellular_params.SIM_SLOT)
#if (CST_SETUP_NFMC == 1)
  PrintCellularService("NFMC activation : %ld\n\r",  CST_nfmc_context.active)

  if (CST_nfmc_context.active == 1U)
  {
    for (uint32_t i = 0U; i < CST_NFMC_TEMPO_NB ; i++)
    {
      PrintCellularService("NFMC value %ld : %ld\n\r", i + 1U, CST_nfmc_context.value[i])
    }
  }
#endif
}
#else /* (CST_SETUP_NFMC == 1) */

static void CST_local_setup_handler(void)
{
  if (CST_default_setup_table[CST_PARAM_PDN_MODE][0] == '1')
  {
    cst_context.set_pdn_mode = 1U;
  }
  else
  {
    cst_context.set_pdn_mode = 0U;
  }
  CST_cellular_params.APN = CST_default_setup_table[CST_PARAM_APN];
  CST_cellular_params.CID = (CS_PDN_conf_id_t)(CST_default_setup_table[CST_PARAM_CID][0] - '0');

  CST_cellular_params.SIM_SLOT = (CST_sim_slot_type_t)(CST_default_setup_table[CST_PARAM_SIM_SLOT][0] - '0');


#if (CST_SETUP_NFMC == 1)
  if (CST_default_setup_table[CST_PARAM_NFMC][0] == '1')
  {
    CST_nfmc_context.active = 1U;
  }
  else
  {
    CST_nfmc_context.active = 0U;
  }

  for (uint32_t i = 0U; i < CST_NFMC_TEMPO_NB ; i++)
  {
    CST_nfmc_context.value[i] = (uint32_t)atoi((char *)CST_default_setup_table[CST_PARAM_NFMC_TEMPO]);
  }
#else
  CST_nfmc_context.active = 1;
  CST_nfmc_context.value[0] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO1_STRING);
  CST_nfmc_context.value[1] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO2_STRING);
  CST_nfmc_context.value[2] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO3_STRING);
  CST_nfmc_context.value[3] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO4_STRING);
  CST_nfmc_context.value[4] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO5_STRING);
  CST_nfmc_context.value[5] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO6_STRING);
  CST_nfmc_context.value[6] = (uint32_t)atoi((char *)CST_DEFAULT_NFMC_TEMPO7_STRING);
#endif /* (CST_SETUP_NFMC == 1) */
}
#endif /* (!USE_DEFAULT_SETUP == 1) */

static void CST_select_sim(CST_sim_slot_type_t sim_slot)
{
  switch (sim_slot)
  {

    case CST_SIM_SLOT_MODEM_EMBEDDED_SIM:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_RESET);
      PrintCellularService("STM32 ESIM SELECTED \r\n")
      break;

    case CST_SIM_SLOT_MODEM_SOCKET:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_RESET);
      PrintCellularService("MODEM SIM SOCKET SELECTED \r\n")
      break;

    case CST_SIM_SLOT_STM32_EMBEDDED_SIM:
    default:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_SET);

      PrintCellularService("MODEM SIM ESIM SELECTED \r\n")
      break;
  }
}

static void CST_reset_fail_count(void)
{
  cst_context.power_on_reset_count       = 0U;
  cst_context.reset_reset_count          = 0U;
  cst_context.init_modem_reset_count     = 0U;
  cst_context.csq_reset_count            = 0U;
  cst_context.gns_reset_count            = 0U;
  cst_context.attach_reset_count         = 0U;
  cst_context.activate_pdn_reset_count   = 0U;
  cst_context.cellular_data_retry_count  = 0U;
  cst_context.global_retry_count         = 0U;
}

/* PDN event callback */
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event)
{
  PrintCellularService("====================================CST_pdn_event_callback (cid=%d / event=%d)\n\r",
                       cid, pdn_event)
  cst_context.pdn_status = pdn_event;
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDN_STATUS_EVENT);

}

/* URC callback */
static void CST_network_reg_callback(void)
{
  PrintCellularService("==================================CST_network_reg_callback\n\r")
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
}

static void CST_location_info_callback(void)
{
  PrintCellularService("CST_location_info_callback\n\r")
}

static void CST_modem_event_callback(CS_ModemEvent_t event)
{
  dc_cellular_data_info_t cellular_data_info;

  /* event is a bitmask, we can have more than one evt reported at the same time */
  if ((event & CS_MDMEVENT_BOOT) != 0)
  {
    PrintCellularService("Modem event received: CS_MDMEVENT_BOOT\n\r")
  }
  if ((event & CS_MDMEVENT_POWER_DOWN) != 0)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_POWER_DOWN\n\r")
  }
  if ((event & CS_MDMEVENT_FOTA_START) != 0)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_FOTA_START\n\r")
    dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
    cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
    cst_context.current_state = CST_MODEM_REPROG_STATE;
    dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
    osTimerStart(CST_fota_timer_handle, CST_FOTA_TIMEOUT);
  }
  if ((event & CS_MDMEVENT_FOTA_END) != 0)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_FOTA_END\n\r")

    /* TRIGGER PLATFORM RESET after a delay  */
    PrintCellularService("TRIGGER PLATFORM REBOOT AFTER FOTA UPDATE ...\n\r")
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 1, ERROR_FATAL);
  }
}

static void  CST_data_cache_set(dc_service_rt_state_t dc_service_state)
{
  dc_cellular_data_info_t cellular_data_info;

  dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
  cellular_data_info.rt_state = dc_service_state;
  dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
}

/* failing configuration management */
static void CST_config_fail_mngt(char *msg_fail, CST_fail_cause_t fail_cause, uint16_t *fail_count, uint16_t fail_max)
{
  PrintCellularService("=== %s Fail !!! === \r\n", msg_fail)
  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 1, ERROR_WARNING);

  *fail_count = *fail_count + 1U;
  cst_context.global_retry_count++;

  CST_data_cache_set(DC_SERVICE_OFF);
  if ((*fail_count <= fail_max) && (cst_context.global_retry_count <= CST_GLOBAL_RETRY_MAX))
  {
    cst_context.current_state = CST_MODEM_RESET_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_INIT_EVENT);
  }
  else
  {
    cst_context.current_state = CST_MODEM_FAIL_STATE;
    cst_context.fail_cause    = CST_MODEM_POWER_ON_FAIL;

    PrintCellularServiceErr("=== CST_set_fail_state %d - count %d/%d FATAL !!! ===\n\r", fail_cause, cst_context.global_retry_count, *fail_count)
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 1, ERROR_FATAL);
  }
}

/* init modem processing */
static void CST_modem_init(void)
{
  CS_init();
}

/* start modem processing */
static void CST_modem_start(void)
{
  if (atcore_task_start(ATCORE_THREAD_STACK_PRIO, ATCORE_THREAD_STACK_SIZE) != ATSTATUS_OK)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 2, ERROR_WARNING);
  }
}

/* init modem processing */
static CS_Status_t CST_set_signal_quality(void)
{
  CS_Status_t cs_status = CELLULAR_ERROR;
  CS_SignalQuality_t sig_quality;
  if (osCS_get_signal_quality(&sig_quality) == CELLULAR_OK)
  {
    CST_csq_count_fail = 0U;
    if ((sig_quality.rssi != cst_context.signal_quality.rssi) || (sig_quality.ber != cst_context.signal_quality.ber))
    {
      cst_context.signal_quality.rssi = sig_quality.rssi;
      cst_context.signal_quality.ber  = sig_quality.ber;

      dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      /* if((sig_quality.rssi == 0) || (sig_quality.rssi == CST_BAD_SIG_RSSI)) */
      if (sig_quality.rssi == CST_BAD_SIG_RSSI)
      {
        cst_cellular_info.cs_signal_level    = DC_NO_ATTACHED;
        cst_cellular_info.cs_signal_level_db = (int32_t)DC_NO_ATTACHED;
      }
      else
      {
        cs_status = CELLULAR_OK;
        CST_signal_quality = sig_quality.rssi;
        cst_cellular_info.cs_signal_level     = sig_quality.rssi;             /*  range 0..99 */
        cst_cellular_info.cs_signal_level_db  = (-113 + 2 * (int32_t)sig_quality.rssi); /* dBm */
      }
      dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
    }

    PrintCellularService(" -Sig quality rssi : %d\n\r", sig_quality.rssi)
    PrintCellularService(" -Sig quality ber  : %d\n\r", sig_quality.ber)
  }
  else
  {
    CST_csq_count_fail++;
    PrintCellularService("Modem signal quality error\n\r")
    if (CST_csq_count_fail >= CST_COUNT_FAIL_MAX)
    {
      PrintCellularService("Modem signal quality error max\n\r")
      CST_csq_count_fail = 0U;
      CST_config_fail_mngt("CS_get_signal_quality",
                           CST_MODEM_CSQ_FAIL,
                           &cst_context.csq_reset_count,
                           CST_CSQ_MODEM_RESET_MAX);
    }
  }
  return cs_status;
}

/* send message to cellular service task */
static CS_Status_t  CST_send_message(uint16_t  type, uint16_t cmd)
{
  CST_message_t cmd_message;
  cmd_message.type = type;
  cmd_message.id   = cmd;

  uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
  osMessagePut((osMessageQId)cst_queue_id, *cmd_message_p, 0U);

  return CELLULAR_OK;
}

static uint64_t CST_util_ipow(uint32_t base, uint32_t exp)
{
  /* implementation of power function */
  uint64_t result = 1U;
  while (exp)
  {
    if ((exp & 1U) != 0U)
    {
      result *= base;
    }
    exp >>= 1;
    base *= base;
  }

  return result;
}

static uint64_t CST_util_convertStringToInt64(uint8_t *p_string, uint32_t size)
{
  uint16_t idx;
  uint16_t nb_digit_ignored = 0U;
  uint16_t loop = 0U;
  uint64_t conv_nbr = 0U;


  /* decimal value */
  for (idx = 0U; idx < size; idx++)
  {
    loop++;
    conv_nbr = conv_nbr + (uint64_t)(((uint64_t)p_string[idx] - 48U) * CST_util_ipow(10U, (size - loop - nb_digit_ignored)));
  }

  return (conv_nbr);
}

static uint32_t CST_calculate_tempo_value(uint32_t nb, uint32_t value, uint64_t imsi)
{
  return value + (uint32_t)(imsi % value);
}

static void CST_fill_nfmc_tempo(uint64_t imsi_p)
{
  uint32_t i;
  dc_nfmc_info_t nfmc_info;

  if (CST_nfmc_context.active != 0U)
  {
    nfmc_info.activate = 1U;
    for (i = 0U ; i < CST_NFMC_TEMPO_NB; i++)
    {
      CST_nfmc_context.tempo[i] = CST_calculate_tempo_value(i, CST_nfmc_context.value[i], imsi_p);
      nfmc_info.tempo[i] = CST_nfmc_context.tempo[i];
      PrintCellularService("VALUE/TEMPO %ld/%ld\n\r",  CST_nfmc_context.value[i], CST_nfmc_context.tempo[i])
    }
    nfmc_info.rt_state = DC_SERVICE_ON;
  }
  else
  {
    nfmc_info.activate = 0U;
    nfmc_info.rt_state = DC_SERVICE_OFF;
    PrintCellularService("CST_fill_nfmc_tempo => DC_SERVICE_OFF\n\r");
  }
  dc_com_write(&dc_com_db, DC_COM_NFMC_TEMPO, (void *)&nfmc_info, sizeof(nfmc_info));

}

/* update device info in data cache */
static void CST_get_device_all_infos(void)
{
  CS_DeviceInfo_t   cst_device_info;
  CS_Status_t       cs_status;
  uint16_t          sim_poll_count;
  uint16_t          end_of_loop;

  sim_poll_count = 0U;
  memset((void *)&cst_device_info, 0, sizeof(CS_DeviceInfo_t));

  cst_device_info.field_requested = CS_DIF_IMEI_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -IMEI: %s\n\r", cst_device_info.u.imei)
  }
  else
  {
    PrintCellularService("IMEI error\n\r")
  }

  dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  cst_device_info.field_requested = CS_DIF_MANUF_NAME_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -MANUFACTURER: %s\n\r", cst_device_info.u.manufacturer_name)
  }
  else
  {
    PrintCellularService("Manufacturer Name error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_MODEL_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -MODEL: %s\n\r", cst_device_info.u. model)
  }
  else
  {
    PrintCellularService("Model error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_REV_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -REVISION: %s\n\r", cst_device_info.u.revision)
  }
  else
  {
    PrintCellularService("Revision error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_SN_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -SERIAL NBR: %s\n\r", cst_device_info.u.serial_number)
  }
  else
  {
    PrintCellularService("Serial Number error\n\r")
  }

  end_of_loop = 1U;
  if (cst_context.sim_mode == CELLULAR_SIM_NOT_INSERTED)
  {
    cst_sim_info.rt_state   = DC_SERVICE_ON;
    cst_sim_info.sim_status = DC_SIM_NOT_INSERTED;
    dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  }
  else
  {
    while (end_of_loop)
    {
      cst_device_info.field_requested = CS_DIF_IMSI_PRESENT;
      cs_status = osCDS_get_device_info(&cst_device_info);
      if (cs_status == CELLULAR_OK)
      {
        CST_IMSI = CST_util_convertStringToInt64(cst_device_info.u.imsi, 15U);
        PrintCellularService(" -IMSI: %llu\n\r", CST_IMSI)
        CST_fill_nfmc_tempo(CST_IMSI);

        memcpy(cst_sim_info.imsi, cst_device_info.u.imsi, DC_MAX_SIZE_IMSI);
        cst_sim_info.sim_status = DC_SIM_OK;
        cst_sim_info.rt_state   = DC_SERVICE_ON;
        dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
        end_of_loop = 0U;
      }
      else if ((cs_status == CELLULAR_SIM_BUSY) || (cs_status == CELLULAR_SIM_ERROR))
      {
        osDelay(100U);
        sim_poll_count++;
        if (sim_poll_count > CST_SIM_POLL_COUNT)
        {
          dc_cs_sim_status_t sim_error;
          if (cs_status == CELLULAR_SIM_BUSY)
          {
            sim_error = DC_SIM_BUSY;
          }
          else
          {
            sim_error = DC_SIM_ERROR;
          }

          cst_sim_info.rt_state   = DC_SERVICE_ON;
          cst_sim_info.sim_status = sim_error;

          dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
          end_of_loop = 0U;
        }
      }
      else
      {
        cst_sim_info.rt_state   = DC_SERVICE_ON;
        if (cs_status == CELLULAR_SIM_NOT_INSERTED)
        {
          cst_sim_info.sim_status = DC_SIM_NOT_INSERTED;
        }
        else if (cs_status == CELLULAR_SIM_PIN_OR_PUK_LOCKED)
        {
          cst_sim_info.sim_status = DC_SIM_PIN_OR_PUK_LOCKED;
        }
        else if (cs_status == CELLULAR_SIM_INCORRECT_PASSWORD)
        {
          cst_sim_info.sim_status = DC_SIM_INCORRECT_PASSWORD;
        }
        else
        {
          cst_sim_info.sim_status = DC_SIM_ERROR;
        }

        dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
        end_of_loop = 0U;
      }
    }
  }
  cst_device_info.field_requested = CS_DIF_PHONE_NBR_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    PrintCellularService(" -PHONE NBR: %s\n\r", cst_device_info.u.phone_number)
  }
  else
  {
    PrintCellularService("Phone number error\n\r")
  }
  if (cst_cellular_info.rt_state == DC_SERVICE_ON)
  {
    dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
  }

}

/* subscribe to network event */
static void CST_subscribe_all_net_events(void)
{
  PrintCellularService("Subscribe URC events: Network registration\n\r")
  osCDS_subscribe_net_event(CS_URCEVENT_CS_NETWORK_REG_STAT, CST_network_reg_callback);
  osCDS_subscribe_net_event(CS_URCEVENT_GPRS_NETWORK_REG_STAT, CST_network_reg_callback);
  osCDS_subscribe_net_event(CS_URCEVENT_EPS_NETWORK_REG_STAT, CST_network_reg_callback);
  PrintCellularService("Subscribe URC events: Location info\n\r")
  osCDS_subscribe_net_event(CS_URCEVENT_EPS_LOCATION_INFO, CST_location_info_callback);
  osCDS_subscribe_net_event(CS_URCEVENT_GPRS_LOCATION_INFO, CST_location_info_callback);
  osCDS_subscribe_net_event(CS_URCEVENT_CS_LOCATION_INFO, CST_location_info_callback);
}

/* subscribe to modem event */
static void CST_subscribe_modem_events(void)
{
  PrintCellularService("Subscribe modems events\n\r")
  CS_ModemEvent_t events_mask = (CS_ModemEvent_t)(CS_MDMEVENT_BOOT |
                                                  CS_MDMEVENT_POWER_DOWN |
                                                  CS_MDMEVENT_FOTA_START |
                                                  CS_MDMEVENT_FOTA_END);
  osCDS_subscribe_modem_event(events_mask, CST_modem_event_callback);
}

/* Data cache callback */
static void CST_notif_cb(dc_com_event_id_t dc_event_id, void *private_gui_data)
{

  if (dc_event_id == DC_COM_CELLULAR_DATA_INFO)
  {
    CST_message_t cmd_message;
    uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
    cmd_message.type = CST_MESSAGE_DC_EVENT;
    cmd_message.id   = (uint16_t)dc_event_id;
    osMessagePut(cst_queue_id, *cmd_message_p, 0U);
  }
}

/* Convert received message to automation event */
static CST_autom_event_t CST_get_autom_event(osEvent event)
{
  CST_autom_event_t autom_event;
  CST_message_t  message;
  CST_message_t *message_p;
  dc_cellular_data_info_t cellular_data_info;

  autom_event = CST_NO_EVENT;
  message_p = (CST_message_t *) & (event.value.v);
  message   = *message_p;

  /* 4 types of possible messages: */
  /*
       -> CS automaton event
       -> CS CMD    (ON/OFF)
       -> DC EVENT  (DC_COM_CELLULAR_DATA_INFO: / FAIL)
       -> DC TIMER  (Polling handle)
  */
  if (message.type == CST_MESSAGE_TIMER_EVENT)
  {
    autom_event = CST_POLLING_TIMER;
  }
  else if (message.type == CST_MESSAGE_CS_EVENT)
  {
    autom_event = (CST_autom_event_t)message.id;
  }
  else if (message.type == CST_MESSAGE_URC_EVENT)
  {
    autom_event = CST_MODEM_URC;
  }
  else if (message.type == CST_MESSAGE_CMD)
  {
    switch (message.id)
    {
      case CST_INIT_EVENT:
      {
        autom_event = (CST_autom_event_t)CST_INIT_EVENT;
        break;
      }
      case CST_MODEM_POWER_ON_EVENT:
      {
        autom_event = (CST_autom_event_t)CST_MODEM_POWER_ON_EVENT;
        break;
      }
      default:
      {
        autom_event = CST_CMD_UNKWONW_EVENT;
        break;
      }
    }
  }
  else if (message.type == CST_MESSAGE_DC_EVENT)
  {
    PrintCellularService("CST_MESSAGE_DC_EVENT\n\r")
    if (message.id == (uint16_t)DC_COM_CELLULAR_DATA_INFO)
    {
      PrintCellularService("CST_MESSAGE_DC_EVENT - 2\n\r")
      dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
      if (DC_SERVICE_FAIL == cellular_data_info.rt_state)
      {
        PrintCellularService("DC_SERVICE_FAIL\n\r")
        autom_event = CST_CELLULAR_DATA_FAIL_EVENT;
      }
    }
  }
  else
  {
    PrintCellularService("CST_get_autom_event : No type matching\n\r")
  }

  return autom_event;
}

/* ===================================================================
   Automaton functions  Begin
   =================================================================== */

/* power on modem processing */
static void CST_power_on_only_modem_mngt(void)
{
  PrintCellularService("*********** CST_power_on_only_modem_mngt ********\n\r")
  osCDS_power_on();
  PrintCellularService("*********** MODEM ON ********\n\r")
}

static void CST_power_on_modem_mngt(void)
{
  CS_Status_t cs_status;
  dc_radio_lte_info_t radio_lte_info;

  PrintCellularService("*********** CST_power_on_modem_mngt ********\n\r")
  cs_status = osCDS_power_on();

#if (CST_RESET_DEBUG == 1)
  if (cst_context.power_on_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail_mngt("CST_power_on_modem_mngt",
                         CST_MODEM_POWER_ON_FAIL,
                         &cst_context.power_on_reset_count,
                         CST_POWER_ON_RESET_MAX);

  }
  else
  {
    /* Data Cache -> Radio ON */
    dc_com_read(&dc_com_db, DC_COM_RADIO_LTE_INFO, (void *)&radio_lte_info, sizeof(radio_lte_info));
    radio_lte_info.rt_state = DC_SERVICE_ON;
    dc_com_write(&dc_com_db, DC_COM_RADIO_LTE_INFO, (void *)&radio_lte_info, sizeof(radio_lte_info));
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWERED_ON_EVENT);
  }
}

static CS_Status_t CST_reset_modem(void)
{
  return osCDS_reset(CS_RESET_AUTO);
}

/* power on modem processing */
static void CST_reset_modem_mngt(void)
{
  CS_Status_t cs_status;
  dc_radio_lte_info_t radio_lte_rt_info;

  PrintCellularService("*********** CST_power_on_modem_mngt ********\n\r")
  cs_status = CST_reset_modem();
#if (CST_RESET_DEBUG == 1)
  if (cst_context.reset_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail_mngt("CST_reset_modem_mngt",
                         CST_MODEM_RESET_FAIL,
                         &cst_context.reset_reset_count,
                         CST_RESET_MAX);

  }
  else
  {
    /* Data Cache -> Radio ON */
    dc_com_read(&dc_com_db, DC_COM_RADIO_LTE_INFO, (void *)&radio_lte_rt_info, sizeof(radio_lte_rt_info));
    radio_lte_rt_info.rt_state = DC_SERVICE_ON;
    dc_com_write(&dc_com_db, DC_COM_RADIO_LTE_INFO, (void *)&radio_lte_rt_info, sizeof(radio_lte_rt_info));
    CST_get_device_all_infos();
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWERED_ON_EVENT);
  }
}

/* init modem management */
static void  CST_init_modem_mngt(void)
{
  CS_Status_t cs_status;

  PrintCellularService("*********** CST_init_modem_mngt ********\n\r")

  cs_status = osCDS_init_modem(CS_CMI_FULL, CELLULAR_FALSE, GOOD_PINCODE);
#if (CST_RESET_DEBUG == 1)
  if (cst_context.init_modem_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif
  if (cs_status == CELLULAR_ERROR)
  {
    CST_config_fail_mngt("CST_init_modem_mngt",
                         CST_MODEM_REGISTER_FAIL,
                         &cst_context.init_modem_reset_count,
                         CST_INIT_MODEM_RESET_MAX);
  }
  else if (cs_status == CELLULAR_SIM_NOT_INSERTED)
  {
    cst_context.sim_mode = CELLULAR_SIM_NOT_INSERTED;
  }
  else
  {
    CST_subscribe_all_net_events();

    /* overwrite operator parameters */
    ctxt_operator.mode   = CS_NRM_AUTO;
    ctxt_operator.format = CS_ONF_NOT_PRESENT;
  }

  CST_get_device_all_infos();
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INITIALIZED_EVENT);
}

/* registration modem management */
static void  CST_net_register_mngt(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t  cst_ctxt_reg_status;

  PrintCellularService("=== CST_net_register_mngt ===\n\r")
  cs_status = osCDS_register_net(&ctxt_operator, &cst_ctxt_reg_status);
  if (cs_status == CELLULAR_OK)
  {
    cst_context.current_EPS_NetworkRegState  = cst_ctxt_reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = cst_ctxt_reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = cst_ctxt_reg_status.CS_NetworkRegState;

    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_EVENT);
  }
  else
  {
    PrintCellularService("===CST_net_register_mngt - FAIL !!! ===\n\r")
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 3, ERROR_WARNING);
  }
}

/* test if modem catch right signal */
static void  CST_signal_quality_test_mngt(void)
{
  PrintCellularService("*********** CST_signal_quality_test_mngt ********\n\r")

  if ((cst_context.signal_quality.rssi != 0U) && (cst_context.signal_quality.rssi != CST_BAD_SIG_RSSI))
  {
    osTimerStart(CST_network_status_timer_handle, CST_NETWORK_STATUS_DELAY);
    PrintCellularService("-----> Start NW REG TIMEOUT TIMER   : %d\n\r", CST_NETWORK_STATUS_DELAY)

    cst_context.current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_EVENT);
  }
}

/* test if network status is OK */
static void  CST_network_status_test_mngt(void)
{
  PrintCellularService("*********** CST_network_status_test_mngt ********\n\r")

#if (CST_TEST_REGISTER_FAIL == 1)
  if (CST_register_fail_test_count >= CST_REGISTER_FAIL_TEST_COUNT_MAX)
#endif
  {
    if ((cst_context.current_EPS_NetworkRegState  == CS_NRS_REGISTERED_HOME_NETWORK)
        || (cst_context.current_EPS_NetworkRegState  == CS_NRS_REGISTERED_ROAMING)
        || (cst_context.current_GPRS_NetworkRegState == CS_NRS_REGISTERED_HOME_NETWORK)
        || (cst_context.current_GPRS_NetworkRegState == CS_NRS_REGISTERED_ROAMING))
    {
      cst_context.current_state = CST_NETWORK_STATUS_OK_STATE;
      cst_context.register_retry_tempo_count = 0U;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
    }
  }
}

static void  CST_network_event_mngt(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t reg_status;

  PrintCellularService("*********** CST_network_event_mngt ********\n\r")
  memset((void *)&reg_status, 0, sizeof(reg_status));
  cs_status = osCDS_get_net_status(&reg_status);
  if (cs_status == CELLULAR_OK)
  {
    cst_context.current_EPS_NetworkRegState  = reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = reg_status.CS_NetworkRegState;

    if ((cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_ROAMING)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_ROAMING))
    {
      CST_data_cache_set(DC_SERVICE_OFF);
      cst_context.current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
    }

    if ((reg_status.optional_fields_presence & CS_RSF_FORMAT_PRESENT) != 0)
    {
      dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
      cst_cellular_info.rt_state              = DC_SERVICE_ON;
      dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      PrintCellularService(" ->operator_name = %s", reg_status.operator_name)
    }

  }
  else
  {
  }

}

/* attach modem management */
static void  CST_attach_modem_mngt(void)
{
  CS_Status_t              cs_status;
  CS_PSattach_t            cst_ctxt_attach_status;
  CS_RegistrationStatus_t  reg_status;

  PrintCellularService("*********** CST_attach_modem_mngt ********\n\r")

  memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));
  cs_status = osCDS_get_net_status(&reg_status);

  if (cs_status == CELLULAR_OK)
  {
    if ((reg_status.optional_fields_presence & CS_RSF_FORMAT_PRESENT) != 0)
    {
      dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
      cst_cellular_info.rt_state              = DC_SERVICE_ON;
      dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      PrintCellularService(" ->operator_name = %s\n\r", reg_status.operator_name)
    }
  }

  cs_status = osCDS_get_attach_status(&cst_ctxt_attach_status);
#if (CST_RESET_DEBUG == 1)
  if (cst_context.attach_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif
  if (cs_status != CELLULAR_OK)
  {
    PrintCellularService("*********** CST_attach_modem_mngt fail ********\n\r")
    CST_config_fail_mngt("CS_get_attach_status FAIL",
                         CST_MODEM_ATTACH_FAIL,
                         &cst_context.attach_reset_count,
                         CST_ATTACH_RESET_MAX);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_FAIL_EVENT);
  }
  else
  {
    if (cst_ctxt_attach_status == CS_PS_ATTACHED)
    {
      PrintCellularService("*********** CST_attach_modem_mngt OK ********\n\r")
      cst_context.current_state = CST_MODEM_REGISTERED_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
    }

    else
    {
      PrintCellularService("===CST_attach_modem_mngt - NOT ATTACHED !!! ===\n\r")
      /* Workaround waiting for Modem behaviour clarification */
      CST_config_fail_mngt("CST_pdn_event_mngt",
                           CST_MODEM_ATTACH_FAIL,
                           &cst_context.power_on_reset_count,
                           CST_POWER_ON_RESET_MAX);
#if 0
      cs_status = osCDS_attach_PS_domain();
      if (cs_status != CELLULAR_OK)
      {
        PrintCellularService("===CST_attach_modem_mngt - osCDS_attach_PS_domain FAIL !!! ===\n\r")
        CST_config_fail_mngt("CS_get_attach_status FAIL",
                             CST_MODEM_ATTACH_FAIL,
                             &cst_context.attach_reset_count,
                             CST_ATTACH_RESET_MAX);
        CST_send_message(CST_MESSAGE_CS_EVENT, CST_FAIL_EVENT);
      }
      else
      {
        osDelay(100);
        CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
      }
#endif
    }
  }
}

/* pdn definition modem management */
static void CST_modem_define_pdn(void)
{
  CS_PDN_configuration_t pdn_conf;

  CS_Status_t cs_status;
  /*
  #if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      cs_status = CELLULAR_OK;
  #else
  */
  /* define user PDN configurations */

  /* common user name and password */
  memset((void *)&pdn_conf, 0, sizeof(CS_PDN_configuration_t));
  memcpy((void *)pdn_conf.username, "", sizeof(""));
  memcpy((void *)pdn_conf.password, "", sizeof(""));

  /* exemple for CS_PDN_USER_CONFIG_1 with access point name =  "PDN CONFIG 1" */
  cs_status = osCDS_define_pdn(CST_cellular_params.CID, (char *)CST_cellular_params.APN, &pdn_conf);

  /* #endif */

#if (CST_RESET_DEBUG == 1)
  if (cst_context.activate_pdn_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail_mngt("CST_modem_define_pdn",
                         CST_MODEM_PDP_DEFINE_FAIL,
                         &cst_context.activate_pdn_reset_count,
                         CST_DEFINE_PDN_RESET_MAX);
  }
  /*
      else
      {
          CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDP_ACTIVATED_EVENT);
      }
  */
}

/* pdn activation modem management */
static CS_Status_t CST_modem_activate_pdn(void)
{
  CS_Status_t cs_status;
  cs_status = osCDS_set_default_pdn(CST_cellular_params.CID);

  /* register to PDN events for this CID*/
  osCDS_register_pdn_event(CST_cellular_params.CID, CST_pdn_event_callback);

  cs_status = osCDS_activate_pdn(CS_PDN_CONFIG_DEFAULT);
#if (CST_TEST_NFMC == 1)
  CST_nfmc_test_count++;
  if (CST_nfmc_test_count < CST_NFMC_TEST_COUNT_MAX)
  {
    cs_status = (CS_Status_t)1;
  }
#endif

#if (CST_RESET_DEBUG == 1)
  if (cst_context.activate_pdn_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif

  if (cs_status != CELLULAR_OK)
  {
    if (CST_nfmc_context.active == 0U)
    {
      osTimerStart(CST_pdn_activate_retry_timer_handle, CST_PDN_ACTIVATE_RETRY_DELAY);
      PrintCellularService("-----> CST_modem_activate_pdn NOK - retry tempo  : %d\n\r", CST_PDN_ACTIVATE_RETRY_DELAY)
    }
    else
    {
      osTimerStart(CST_pdn_activate_retry_timer_handle, CST_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count]);
      PrintCellularService("-----> CST_modem_activate_pdn NOK - retry tempo %d : %ld\n\r", cst_context.activate_pdn_nfmc_tempo_count + 1U,
                           CST_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count])
    }

    cst_context.activate_pdn_nfmc_tempo_count++;
    if (cst_context.activate_pdn_nfmc_tempo_count >= CST_NFMC_TEMPO_NB)
    {
      cst_context.activate_pdn_nfmc_tempo_count = 0U;
    }
  }
  else
  {
    cst_context.activate_pdn_nfmc_tempo_count = 0U;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDP_ACTIVATED_EVENT);
  }
  return cs_status;
}

/*  pppos config fail */
static void CST_cellular_data_fail_mngt(void)
{
  CST_config_fail_mngt("CST_cellular_data_fail_mngt",
                       CST_MODEL_CELLULAR_DATA_FAIL,
                       &cst_context.cellular_data_retry_count,
                       CST_CELLULAR_DATA_RETRY_MAX);
}

static void CST_pdn_event_mngt(void)
{
  if (cst_context.pdn_status == CS_PDN_EVENT_NW_DETACH)
  {
    /* Workaround waiting for Modem behaviour clarification */
    CST_config_fail_mngt("CST_pdn_event_mngt",
                         CST_MODEM_ATTACH_FAIL,
                         &cst_context.power_on_reset_count,
                         CST_POWER_ON_RESET_MAX);
#if 0
    cst_context.current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
#endif
  }
  else if (
    (cst_context.pdn_status == CS_PDN_EVENT_NW_DEACT)
    || (cst_context.pdn_status == CS_PDN_EVENT_NW_PDN_DEACT))
  {
    PrintCellularService("=========CST_pdn_event_mngt CS_PDN_EVENT_NW_DEACT\n\r")
    cst_context.current_state = CST_MODEM_REGISTERED_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
  }
  else
  {
    cst_context.current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
  }
}

/* =================================================================
   Management automaton functions according current automation state
   ================================================================= */

static void CST_init_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_INIT_EVENT:
    {
      cst_context.current_state = CST_MODEM_ON_STATE;
      CST_power_on_modem_mngt();
      break;
    }
    case CST_MODEM_POWER_ON_EVENT:
    {
      cst_context.current_state = CST_MODEM_ON_ONLY_STATE;
      CST_power_on_only_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 7, ERROR_WARNING);
      break;
    }
  }
  /* subscribe modem events after power ON */
  CST_subscribe_modem_events();
}

static void CST_reset_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_INIT_EVENT:
    {
      cst_context.current_state = CST_MODEM_ON_STATE;
      CST_reset_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 7, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_on_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_POWERED_ON_EVENT:
    {
      cst_context.current_state = CST_MODEM_POWERED_ON_STATE;
      CST_init_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 8, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_powered_on_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_INITIALIZED_EVENT:
    {
      if (cst_context.set_pdn_mode)
      {
        PrintCellularService("CST_modem_on_state : CST_modem_define_pdn\n\r")
        CST_modem_define_pdn();
      }
      cst_context.current_state = CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE;
      CST_net_register_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 9, ERROR_WARNING);
      break;
    }
  }
}

static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event)
{

  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    case CST_SIGNAL_QUALITY_EVENT:
    {
      CST_signal_quality_test_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 9, ERROR_WARNING);
      break;
    }
  }
}

static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    case CST_NETWORK_STATUS_EVENT:
    {
      CST_network_status_test_mngt();
      break;
    }
    case CST_NW_REG_TIMEOUT_TIMER_EVENT:
    {
      PrintCellularService("-----> NW REG TIMEOUT TIMER EXPIRY WE PWDN THE MODEM \n\r")
      cst_context.current_state = CST_MODEM_NETWORK_STATUS_FAIL_STATE;
      CS_power_off();

      osTimerStart(CST_register_retry_timer_handle, CST_nfmc_context.tempo[cst_context.register_retry_tempo_count]);
      PrintCellularService("-----> CST_waiting_for_network_status NOK - retry tempo %d : %ld\n\r", cst_context.register_retry_tempo_count + 1U,
                           CST_nfmc_context.tempo[cst_context.register_retry_tempo_count])
#if (CST_TEST_REGISTER_FAIL == 1)
      CST_register_fail_test_count++;
#endif
      cst_context.register_retry_tempo_count++;
      if (cst_context.register_retry_tempo_count >= CST_NFMC_TEMPO_NB)
      {
        cst_context.register_retry_tempo_count = 0U;
      }
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 9, ERROR_WARNING);
      break;
    }
  }
}

static void CST_network_status_ok_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_STATUS_OK_EVENT:
    {
      CST_attach_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 9, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_registered_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_ATTACHED_EVENT:
    {
      cst_context.current_state = CST_MODEM_PDN_ACTIVATE_STATE;
      CST_modem_activate_pdn();
      /* CS_Status_t CS_get_dev_IP_address(CS_PDN_conf_id_t cid, CS_IPaddrType_t *ip_addr_type, char *p_ip_addr_value) */
      break;
    }
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 10, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_pdn_activate_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_PDP_ACTIVATED_EVENT:
    {
      cst_context.current_state = CST_MODEM_DATA_READY_STATE;
      CST_reset_fail_count();
      CST_data_cache_set(DC_SERVICE_ON);
      break;
    }
    case CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT:
    {
      CST_modem_activate_pdn();
      break;
    }
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_PDN_STATUS_EVENT:
    {
      CST_pdn_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 10, ERROR_WARNING);
      break;
    }
  }
}

static void CST_data_ready_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_CELLULAR_DATA_FAIL_EVENT:
    {
      CST_cellular_data_fail_mngt();
      break;
    }
    case CST_PDN_STATUS_EVENT:
    {
      CST_pdn_event_mngt();
      break;
    }

    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 13, ERROR_WARNING);
      break;
    }
  }
}

static void CST_fail_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_FAIL_EVENT:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 25, ERROR_WARNING);
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 26, ERROR_WARNING);
      break;
    }
  }
}

/* ===================================================================
   Automaton functions  END
   =================================================================== */


/* Timer handler
     During configuration :  Signal quality  and network status Polling
     After configuration  :  Modem monitoring (Signal quality checking)
 */
static void CST_timer_handler(void)
{
  CS_Status_t cs_status ;
  CS_RegistrationStatus_t reg_status;

  /*    PrintCellularService("-----> CST_timer_handler  <-----\n\r") */
  CST_polling_timer_flag = 0U;
  if (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));
    cs_status = osCDS_get_net_status(&reg_status);
    if (cs_status == CELLULAR_OK)
    {
      cst_context.current_EPS_NetworkRegState  = reg_status.EPS_NetworkRegState;
      cst_context.current_GPRS_NetworkRegState = reg_status.GPRS_NetworkRegState;
      cst_context.current_CS_NetworkRegState   = reg_status.CS_NetworkRegState;
      PrintCellularService("-----> CST_timer_handler - CST_NETWORK_STATUS_EVENT <-----\n\r")

      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_EVENT);

      if ((reg_status.optional_fields_presence & CS_RSF_FORMAT_PRESENT) != 0)
      {
        dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
        cst_cellular_info.rt_state              = DC_SERVICE_ON;
        dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

        PrintCellularService(" ->operator_name = %s\n\r", reg_status.operator_name)
      }
    }
    else
    {
      CST_config_fail_mngt("osCDS_get_net_status",
                           CST_MODEM_GNS_FAIL,
                           &cst_context.gns_reset_count,
                           CST_GNS_MODEM_RESET_MAX);
    }
  }

  if (cst_context.current_state == CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE)
  {
    CST_set_signal_quality();
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_EVENT);
  }

#if (CST_FOTA_TEST == 1)
  if (cst_context.current_state == CST_MODEM_DATA_READY_STATE)
  {
    CST_modem_event_callback(CS_MDMEVENT_FOTA_START);
  }
#endif

  if ((cst_context.current_state == CST_MODEM_DATA_READY_STATE) && (CST_MODEM_POLLING_PERIOD != 0U))
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    osCDS_suspend_data();
#endif
    CST_set_signal_quality();
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    /* CS_check_connection(); */
    osCDS_resume_data();
#endif
  }
}

/* Cellular Service Task : autmaton management */
static void CST_cellular_service_task(void const *argument)
{
  osEvent event;
  CST_autom_event_t autom_event;

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);


  for (;;)
  {
    event = osMessageGet(cst_queue_id, osWaitForever);
    autom_event = CST_get_autom_event(event);
    /*    PrintCellularService("======== CST_cellular_service_task autom_event %d\n\r", autom_event) */

    if (autom_event == CST_POLLING_TIMER)
    {
      CST_timer_handler();
    }
    else if (autom_event != CST_NO_EVENT)
    {
      switch (cst_context.current_state)
      {
        case CST_MODEM_INIT_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_INIT_STATE <-----\n\r")
          CST_init_state(autom_event);
          break;
        }
        case CST_MODEM_RESET_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_RESET_STATE <-----\n\r")
          CST_reset_state(autom_event);
          break;
        }
        case CST_MODEM_ON_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_ON_STATE <-----\n\r")
          CST_modem_on_state(autom_event);
          break;
        }
        case CST_MODEM_POWERED_ON_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_POWERED_ON_STATE <-----\n\r")
          CST_modem_powered_on_state(autom_event);
          break;
        }
        case CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE:
        {
          PrintCellularService("-----> State : CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE <-----\n\r")
          CST_waiting_for_signal_quality_ok_state(autom_event);
          break;
        }
        case CST_WAITING_FOR_NETWORK_STATUS_STATE:
        {
          PrintCellularService("-----> State : CST_WAITING_FOR_NETWORK_STATUS_STATE <-----\n\r")
          CST_waiting_for_network_status_state(autom_event);
          break;
        }

        case CST_NETWORK_STATUS_OK_STATE:
        {
          PrintCellularService("-----> State : CST_NETWORK_STATUS_OK_STATE <-----\n\r")
          CST_network_status_ok_state(autom_event);
          break;
        }

        case CST_MODEM_REGISTERED_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_REGISTERED_STATE <-----\n\r")
          CST_modem_registered_state(autom_event);
          break;
        }
        case CST_MODEM_PDN_ACTIVATE_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_PDN_ACTIVATE_STATE <-----\n\r")
          CST_modem_pdn_activate_state(autom_event);
          break;
        }

        case CST_MODEM_DATA_READY_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_DATA_READY_STATE <-----\n\r")
          CST_data_ready_state(autom_event);
          break;
        }
        case CST_MODEM_FAIL_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_FAIL_STATE <-----\n\r")
          CST_fail_state(autom_event);
          break;
        }
        default:
        {
          PrintCellularService("-----> State : Not State <-----\n\r")
          break;
        }
      }
    }
    else
    {
      PrintCellularService("============ CST_cellular_service_task : autom_event = no event \n\r")
    }
  }
}

/* CST_polling_timer_callback function */
static void CST_polling_timer_callback(void const *argument)
{
  if (cst_context.current_state != CST_MODEM_INIT_STATE)
  {
    CST_message_t cmd_message;
    uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);

    cmd_message.type = CST_MESSAGE_TIMER_EVENT;
    cmd_message.id   = 0U;
    if (CST_polling_timer_flag == 0U)
    {
      CST_polling_timer_flag = 1U;
      osMessagePut(cst_queue_id, *cmd_message_p, 0U);
    }
  }
}

/* CST_pdn_activate_retry_timer_callback */
static void CST_pdn_activate_retry_timer_callback(void const *argument)
{
  if (cst_context.current_state == CST_MODEM_PDN_ACTIVATE_STATE)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT);
  }
}

/* CST_pdn_activate_retry_timer_callback */
static void CST_register_retry_timer_callback(void const *argument)
{
  if (cst_context.current_state == CST_MODEM_NETWORK_STATUS_FAIL_STATE)
  {
    cst_context.current_state = CST_MODEM_INIT_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_INIT_EVENT);
  }
}

static void CST_network_status_timer_callback(void const *argument)
{
  if (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NW_REG_TIMEOUT_TIMER_EVENT);
  }
}


/* CST_fota_timer_callback */
/* FOTA Timeout expired : restart board */
static void CST_fota_timer_callback(void const *argument)
{
  PrintCellularService("CST FOTA FAIL : Timeout expired RESTART\n\r")

  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 20, ERROR_FATAL);
}


/* Functions Definition ------------------------------------------------------*/
CS_Status_t  CST_radio_on(void)
{
  return CST_send_message(CST_MESSAGE_CMD, CST_INIT_EVENT);
}

CS_Status_t  CST_modem_power_on(void)
{
  return CST_send_message(CST_MESSAGE_CMD, CST_MODEM_POWER_ON_EVENT);
}

CS_Status_t CST_cellular_service_init(void)
{
  cst_context.current_state = CST_MODEM_INIT_STATE;
  CST_modem_init();

  osCDS_cellular_service_init();
  CST_csq_count_fail = 0U;

#if (!USE_DEFAULT_SETUP == 1)
  app_select_record((uint8_t *)CST_MODEM_POWER_ON_LABEL, APP_SELECT_CST_MODEM_START);
  setup_record(SETUP_APPLI_CST, CST_VERSION_APPLI, (uint8_t *)CST_LABEL, CST_setup_handler, CST_setup_dump, CST_default_setup_table, CST_DEFAULT_PARAMA_NB);
#else
  CST_local_setup_handler();
#endif

  osMessageQDef(cst_queue_id, 10, sizeof(CST_message_t));
  cst_queue_id = osMessageCreate(osMessageQ(cst_queue_id), NULL);
  if (cst_queue_id == NULL)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 18, ERROR_FATAL);
  }
  return CELLULAR_OK;
}

CST_state_t CST_get_state(void)
{
  return cst_context.current_state;
}

CS_Status_t CST_cellular_service_start(void)
{
  dc_nfmc_info_t nfmc_info;
  uint32_t cst_polling_period = 0U;

  CST_modem_start();

  CST_select_sim(CST_cellular_params.SIM_SLOT);

  dc_com_register_gen_event_cb(&dc_com_db, CST_notif_cb, (void *)NULL);
  cst_cellular_info.mno_name[0]           = (int8_t)'0';
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;


  dc_com_write(&dc_com_db, DC_COM_CELLULAR, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  nfmc_info.rt_state = DC_SERVICE_UNAVAIL;
  dc_com_write(&dc_com_db, DC_COM_NFMC_TEMPO, (void *)&nfmc_info, sizeof(nfmc_info));

  osTimerDef(cs_polling_timer, CST_polling_timer_callback);
  cst_polling_timer_handle = osTimerCreate(osTimer(cs_polling_timer), osTimerPeriodic, NULL);
  if (CST_MODEM_POLLING_PERIOD == 0U)
  {
    cst_polling_period = CST_MODEM_POLLING_PERIOD_DEFAULT;
  }
  else
  {
    cst_polling_period = CST_MODEM_POLLING_PERIOD;
  }

  osTimerStart(cst_polling_timer_handle, cst_polling_period);

  osTimerDef(CST_pdn_activate_retry_timer, CST_pdn_activate_retry_timer_callback);
  CST_pdn_activate_retry_timer_handle = osTimerCreate(osTimer(CST_pdn_activate_retry_timer), osTimerOnce, NULL);

  osTimerDef(CST_waiting_for_network_status_timer, CST_network_status_timer_callback);
  CST_network_status_timer_handle = osTimerCreate(osTimer(CST_waiting_for_network_status_timer), osTimerOnce, NULL);

  osTimerDef(CST_register_retry_timer, CST_register_retry_timer_callback);
  CST_register_retry_timer_handle = osTimerCreate(osTimer(CST_register_retry_timer), osTimerOnce, NULL);

  osTimerDef(CST_fota_timer, CST_fota_timer_callback);
  CST_fota_timer_handle = osTimerCreate(osTimer(CST_fota_timer), osTimerOnce, NULL);

  osThreadDef(cellularServiceTask, CST_cellular_service_task, CELLULAR_SERVICE_THREAD_PRIO, 0, CELLULAR_SERVICE_THREAD_STACK_SIZE);
  CST_cellularServiceThreadId = osThreadCreate(osThread(cellularServiceTask), NULL);

  if (CST_cellularServiceThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 19, ERROR_FATAL);
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(CST_cellularServiceThreadId, CELLULAR_SERVICE_THREAD_STACK_SIZE);
#endif /* STACK_ANALYSIS_TRACE */
  }

  return CELLULAR_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

