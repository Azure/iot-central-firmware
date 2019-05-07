/**
  ******************************************************************************
  * @file    cellular_init.c
  * @author  MCD Application Team
  * @brief   Initialisation of cellular components
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
#include "cmsis_os.h"
#include "board_buttons.h"

#include "cellular_init.h"

#include "ipc_uart.h"
#include "com_sockets.h"
#include "cellular_service.h"
#include "cellular_service_task.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif_client.h"
#endif
#include "nifman.h"
#include "dc_common.h"
#include "radio_mngt.h"
#include "plf_config.h"

#if (USE_TRACE_TEST == 1U)
#include "trace_interface.h"
#endif

/* Private defines -----------------------------------------------------------*/
#define STACK_ANALYSIS_COUNTER  50U  /*  Stack analysed every 5s */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_TEST == 1U)
#if (USE_PRINTF == 0U)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_MAIN, DBL_LVL_P0, "RTOS:" (format) "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_MAIN, DBL_LVL_ERR, "RTOS ERROR:" (format) "\n\r", ## args)
#else
#define PrintDBG(format, args...)   printf("RTOS:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("RTOS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintDBG(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_TEST */

/* Private variables ---------------------------------------------------------*/
static uint32_t jiffies_loc = 0U;

static dc_cellular_info_t          dc_cellular_info;
static dc_ppp_client_info_t        dc_ppp_client_info;
static dc_cellular_data_info_t     dc_cellular_data_info;
static dc_radio_lte_info_t         dc_radio_lte_info;
static dc_nifman_info_t            dc_nifman_info;
static dc_nfmc_info_t              dc_nfmc_info;
static dc_sim_info_t               dc_sim_info;


/* Private typedef -----------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void dc_cellular_init(void);

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t    DC_COM_CELLULAR       = -1;
dc_com_res_id_t    DC_COM_PPP_CLIENT     = -1;
dc_com_res_id_t    DC_COM_CELLULAR_DATA  = -1;
dc_com_res_id_t    DC_COM_RADIO_LTE      = -1;
dc_com_res_id_t    DC_COM_NIFMAN         = -1;
dc_com_res_id_t    DC_COM_NFMC_TEMPO     = -1;
dc_com_res_id_t    DC_COM_SIM_INFO       = -1;



/* Functions Definition ------------------------------------------------------*/

static void dc_cellular_init(void)
{
  memset((void *)&dc_cellular_info,      0, sizeof(dc_cellular_info_t));
  memset((void *)&dc_ppp_client_info,    0, sizeof(dc_ppp_client_info_t));
  memset((void *)&dc_cellular_data_info, 0, sizeof(dc_cellular_data_info_t));
  memset((void *)&dc_radio_lte_info,     0, sizeof(dc_radio_lte_info_t));
  memset((void *)&dc_nifman_info,        0, sizeof(dc_nifman_info_t));
  memset((void *)&dc_nfmc_info,          0, sizeof(dc_nfmc_info_t));
  memset((void *)&dc_sim_info,           0, sizeof(dc_sim_info_t));

  DC_COM_CELLULAR      = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_info,      sizeof(dc_cellular_info_t));
  DC_COM_PPP_CLIENT    = dc_com_register_serv(&dc_com_db, (void *)&dc_ppp_client_info,    sizeof(dc_ppp_client_info_t));
  DC_COM_CELLULAR_DATA = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_data_info, sizeof(dc_cellular_data_info_t));
  DC_COM_RADIO_LTE     = dc_com_register_serv(&dc_com_db, (void *)&dc_radio_lte_info,     sizeof(dc_radio_lte_info_t));
  DC_COM_NIFMAN        = dc_com_register_serv(&dc_com_db, (void *)&dc_nifman_info,        sizeof(dc_nifman_info_t));
  DC_COM_NFMC_TEMPO    = dc_com_register_serv(&dc_com_db, (void *)&dc_nfmc_info,          sizeof(dc_nfmc_info_t));
  DC_COM_SIM_INFO      = dc_com_register_serv(&dc_com_db, (void *)&dc_sim_info,           sizeof(dc_sim_info_t));
}

void cellular_init(void)
{
  dc_com_start();

  dc_com_init();

  dc_cellular_init();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  ppposif_client_init();
#endif

  nifman_init();
}


void cellular_start(void)
{
  com_start();

  dc_com_start();
  

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  ppposif_client_start();
#endif

  nifman_start();

  /* dynamical init of components */
#if defined(AT_TEST)
  at_modem_start();
#endif

#if !defined(AT_TEST)
  CST_cellular_service_start();
#endif


#if !defined(AT_TEST)
  radio_mngt_radio_on();
#endif
}

void cellular_modem_start(void)
{
  CST_cellular_service_start();
  CST_modem_power_on();
}


/* sys_jiffies needed by magic.c module */
uint32_t sys_jiffies(void)
{
  jiffies_loc += 10U;
  return jiffies_loc;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
