/**
  ******************************************************************************
  * @file    plf_features.h
  * @author  MCD Application Team
  * @brief   Includes feature list to include in firmware
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
#ifndef PLF_FEATURES_H
#define PLF_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* ================================================= */
/*          USER MODE                                */
/* ================================================= */

/* ===================================== */
/* BEGIN - Cellular data mode            */
/* ===================================== */
/* Possible values for USE_SOCKETS_TYPE */
#define USE_SOCKETS_LWIP   (0)  /* define value affected to LwIP sockets type */
#define USE_SOCKETS_MODEM  (1)  /* define value affected to Modem sockets type */
/* Sockets location */
#define USE_SOCKETS_TYPE   (USE_SOCKETS_MODEM)

/* If activated then com_ping interfaces in com_sockets module are defined
   mandatory when USE_PING_CLIENT is defined */
#define USE_COM_PING       (1)  /* 0: not activated, 1: activated */

/* If activated then for USE_SOCKETS_TYPE == USE_SOCKETS_MODEM
   com_getsockopt with COM_SO_ERROR parameter return a value compatible with errno.h
   see com_sockets_err_compat.c for the conversion */
#define COM_SOCKETS_ERRNO_COMPAT (0) /* 0: not activated, 1: activated */

/* ===================================== */
/* END - Cellular data mode              */
/* ===================================== */

/* ===================================== */
/* BEGIN - Applications to include       */
/* ===================================== */
#define USE_HTTP_CLIENT    (1) /* 0: not activated, 1: activated */
#define USE_PING_CLIENT    (1) /* 0: not activated, 1: activated */
#define USE_DC_EMUL        (1) /* 0: not activated, 1: activated */
#define USE_DC_TEST        (0) /* 0: not activated, 1: activated */

/* MEMS setup */
#define USE_DC_MEMS        (1) /* 0: not activated, 1: activated */
#define USE_SIMU_MEMS      (0) /* 0: not activated, 1: activated */

#if ( USE_PING_CLIENT == 1 ) && ( USE_COM_PING == 0 )
#error USE_COM_PING must be set to 1 when Ping Client is activated.
#endif

/* ===================================== */
/* END   - Applications to include       */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */
/* To configure some parameters of the software */
#define USE_DEFAULT_SETUP     (1) /* 0: Use setup menu,
                                     1: Use default parameters, no setup menu */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_FEATURES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
