/**
  ******************************************************************************
  * @file    dc_cellular.h
  * @author  MCD Application Team
  * @brief   Data Dache definitions for cellular components
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
#ifndef DC_CELLULAR_H
#define DC_CELLULAR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dc_common.h"
#include "cellular_service.h"
#include "com_sockets.h"

/* Exported constants --------------------------------------------------------*/
#define DC_MAX_SIZE_MNO_NAME       32U
#define DC_MAX_SIZE_MANUFACT_NAME  32U
#define DC_MAX_SIZE_IMSI           32U
#define DC_NFMC_TEMPO_NB          7

/* Exported types ------------------------------------------------------------*/

#define DC_NO_ATTACHED 0U

typedef uint8_t dc_cs_signal_level_t;   /*  range 0..99  (0: DC_NO_ATTACHED) */

typedef enum
{
  DC_SIM_OK                  = 0,
  DC_SIM_ERROR               = 1,
  DC_SIM_BUSY                = 2,
  DC_SIM_NOT_INSERTED        = 3,
  DC_SIM_PIN_OR_PUK_LOCKED   = 4,
  DC_SIM_INCORRECT_PASSWORD  = 5
} dc_cs_sim_status_t;

typedef com_ip_addr_t dc_network_addr_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  int8_t mno_name[DC_MAX_SIZE_MNO_NAME];
  uint32_t    cs_signal_level;
  int32_t     cs_signal_level_db;
} dc_cellular_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  int8_t            imsi[DC_MAX_SIZE_IMSI];
  dc_cs_sim_status_t  sim_status;
} dc_sim_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
} dc_cellular_data_info_t;

typedef enum
{
  DC_NO_NETWORK         = 0,
  DC_CELLULAR_NETWORK   = 1,
  DC_WIFI_NETWORK       = 2
} dc_nifman_network_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  dc_nifman_network_t    network;
  dc_network_addr_t ip_addr;
  dc_network_addr_t netmask;
  dc_network_addr_t gw;
} dc_nifman_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
} dc_radio_lte_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  dc_network_addr_t ip_addr;
  dc_network_addr_t netmask;
  dc_network_addr_t gw;
} dc_ppp_client_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  uint32_t activate;
  uint32_t tempo[DC_NFMC_TEMPO_NB];
}   dc_nfmc_info_t;


/* External variables --------------------------------------------------------*/
extern dc_com_res_id_t    DC_COM_CELLULAR      ;
extern dc_com_res_id_t    DC_COM_PPP_CLIENT    ;
extern dc_com_res_id_t    DC_COM_CELLULAR_DATA ;
extern dc_com_res_id_t    DC_COM_RADIO_LTE     ;
extern dc_com_res_id_t    DC_COM_NIFMAN        ;
extern dc_com_res_id_t    DC_COM_NFMC_TEMPO    ;
extern dc_com_res_id_t    DC_COM_SIM_INFO      ;

#define DC_COM_CELLULAR_INFO        DC_COM_CELLULAR
#define DC_COM_PPP_CLIENT_INFO      DC_COM_PPP_CLIENT
#define DC_COM_CELLULAR_DATA_INFO   DC_COM_CELLULAR_DATA
#define DC_COM_RADIO_LTE_INFO       DC_COM_RADIO_LTE
#define DC_COM_NIFMAN_INFO          DC_COM_NIFMAN
#define DC_COM_NFMC_TEMPO_INFO      DC_COM_NFMC_TEMPO

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */



#ifdef __cplusplus
}
#endif

#endif /* DC_CELLULAR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
