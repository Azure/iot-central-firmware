/**
  ******************************************************************************
  * @file    sysctrl.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl.c module
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
#ifndef SYSCTRL_H
#define SYSCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
#define MAX_CONNECTED_DEVICES (2U) /* maximum number of hardwares devices (modem, wifi,...) connected */

/* Exported types ------------------------------------------------------------*/
typedef uint16_t sysctrl_handle_t;

typedef enum
{
  SCSTATUS_OK = 0,
  SCSTATUS_ERROR,
} sysctrl_status_t;

typedef enum
{
  DEVTYPE_MODEM_CELLULAR = 0,
  /*  DEVTYPE_WIFI, */
  /*  DEVTYPE_GPS, */
  /* etc... all modules using AT commands */

  /* --- */
  DEVTYPE_INVALID,      /* keep it last */
} sysctrl_device_type_t;

typedef struct
{
  sysctrl_device_type_t       type;
  IPC_Device_t                ipc_device;
  IPC_Interface_t             ipc_interface;
} sysctrl_info_t;

typedef sysctrl_status_t (*SC_getDeviceDescriptor)(sysctrl_device_type_t device_type, sysctrl_info_t *p_devices_list);
typedef sysctrl_status_t (*SC_power_on)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_power_off)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_power_reset_device)(sysctrl_device_type_t device_type);

typedef struct
{
  uint8_t                     initialized;
  SC_getDeviceDescriptor      f_getDeviceDescriptor;
  SC_power_on                 f_power_on;
  SC_power_off                f_power_off;
  SC_power_reset_device       f_reset_device;
} sysctrl_funcPtrs_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
sysctrl_status_t SysCtrl_getDeviceDescriptor(sysctrl_device_type_t device_type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_power_on(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_power_off(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_reset_device(sysctrl_device_type_t device_type);

#ifdef __cplusplus
}
#endif

#endif /* SYSCTRL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
