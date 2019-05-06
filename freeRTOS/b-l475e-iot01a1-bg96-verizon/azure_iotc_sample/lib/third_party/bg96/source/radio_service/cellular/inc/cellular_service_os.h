/**
  ******************************************************************************
  * @file    cellular_service_os.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_os.c module
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
#ifndef CELLULAR_SERVICE_OS_H
#define CELLULAR_SERVICE_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "cellular_service.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/* Cellular Service Library Init */
CS_Bool_t osCDS_cellular_service_init(void);

CS_Status_t osCS_get_signal_quality(CS_SignalQuality_t *p_sig_qual);

/* SOCKET API */
socket_handle_t osCDS_socket_create(CS_IPaddrType_t addr_type,
                                    CS_TransportProtocol_t protocol,
                                    CS_PDN_conf_id_t cid);

CS_Status_t osCDS_socket_set_callbacks(socket_handle_t sockHandle,
                                       cellular_socket_data_ready_callback_t data_ready_cb,
                                       cellular_socket_data_sent_callback_t data_sent_cb,
                                       cellular_socket_closed_callback_t remote_close_cb);

CS_Status_t osCDS_socket_set_option(socket_handle_t sockHandle,
                                    CS_SocketOptionLevel_t opt_level,
                                    CS_SocketOptionName_t opt_name,
                                    void *p_opt_val);

CS_Status_t osCDS_socket_get_option(void);

CS_Status_t osCDS_socket_bind(socket_handle_t sockHandle,
                              uint16_t local_port);

CS_Status_t osCDS_socket_connect(socket_handle_t sockHandle,
                                 CS_IPaddrType_t addr_type,
                                 CS_CHAR_t *p_ip_addr_value,
                                 uint16_t remote_port); /* for socket client mode */

CS_Status_t osCDS_socket_listen(socket_handle_t sockHandle);

CS_Status_t osCDS_socket_send(socket_handle_t sockHandle,
                              const CS_CHAR_t *p_buf,
                              uint32_t length);

int32_t osCDS_socket_receive(socket_handle_t sockHandle,
                             CS_CHAR_t *p_buf,
                             uint32_t max_buf_length);

CS_Status_t osCDS_socket_cnx_status(socket_handle_t sockHandle,
                                    CS_SocketCnxInfos_t *infos);

CS_Status_t osCDS_socket_close(socket_handle_t sockHandle,
                               uint8_t force);

/* =========================================================
   ===========      Mode Command services        ===========
   ========================================================= */
CS_Status_t osCDS_get_net_status(CS_RegistrationStatus_t *p_reg_status);
CS_Status_t osCDS_get_device_info(CS_DeviceInfo_t *p_devinfo);
CS_Status_t osCDS_subscribe_net_event(CS_UrcEvent_t event,
                                      cellular_urc_callback_t urc_callback);
CS_Status_t osCDS_subscribe_modem_event(CS_ModemEvent_t events_mask,
                                        cellular_modem_event_callback_t modem_evt_cb);
CS_Status_t osCDS_power_on(void);
CS_Status_t osCDS_reset(CS_Reset_t rst_type);
CS_Status_t osCDS_init_modem(CS_ModemInit_t init,
                             CS_Bool_t reset,
                             const char *pin_code);
CS_Status_t osCDS_register_net(CS_OperatorSelector_t *p_operator,
                               CS_RegistrationStatus_t *p_reg_status);
CS_Status_t osCDS_get_attach_status(CS_PSattach_t *p_attach);
CS_Status_t osCDS_attach_PS_domain(void);

CS_Status_t osCDS_define_pdn(CS_PDN_conf_id_t cid,
                             const char *apn,
                             CS_PDN_configuration_t *pdn_conf);
CS_Status_t osCDS_register_pdn_event(CS_PDN_conf_id_t cid,
                                     cellular_pdn_event_callback_t pdn_event_callback);
CS_Status_t osCDS_set_default_pdn(CS_PDN_conf_id_t cid);
CS_Status_t osCDS_activate_pdn(CS_PDN_conf_id_t cid);

CS_Status_t osCDS_suspend_data(void);
CS_Status_t osCDS_resume_data(void);

CS_Status_t osCDS_dns_request(CS_PDN_conf_id_t cid,
                              CS_DnsReq_t  *dns_req,
                              CS_DnsResp_t *dns_resp);

CS_Status_t osCDS_ping(CS_PDN_conf_id_t cid,
                       CS_Ping_params_t *ping_params,
                       cellular_ping_response_callback_t cs_ping_rsp_cb);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_OS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
