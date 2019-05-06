/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
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
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
#include "main.h"
#include "plf_modem_config.h"

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/

/* MODEM configuration */
#define MODEM_UART_HANDLE       huart4
#define MODEM_UART_INSTANCE     UART4
#define MODEM_UART_AUTOBAUD     (1)
  
#define MODEM_UART_BAUDRATE     (CONFIG_MODEM_UART_BAUDRATE)
#if (CONFIG_MODEM_UART_RTS_CTS == 1)
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_RTS_CTS
#else
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_NONE
#endif
#define MODEM_UART_WORDLENGTH   UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS     UART_STOPBITS_1
#define MODEM_UART_PARITY       UART_PARITY_NONE
#define MODEM_UART_MODE         UART_MODE_TX_RX

/* ---- MODEM other pins configuration ---- */
#define MODEM_RST_GPIO_Port             GPIOA       /* MDM_RST_GPIO_Port */
#define MODEM_RST_Pin                   GPIO_PIN_15  /* MDM_RST_Pin */
#define MODEM_PWR_EN_GPIO_Port          GPIOB       /* MDM_PWR_EN_GPIO_Port */
#define MODEM_PWR_EN_Pin                GPIO_PIN_4  /* MDM_PWR_EN_Pin */
#define MODEM_DTR_GPIO_Port             GPIOB       /* MDM_DTR_GPIO_Port */
#define MODEM_DTR_Pin                   GPIO_PIN_1   /* MDM_DTR_Pin */
  
#define PPPOS_LINK_UART_HANDLE   NULL
#define PPPOS_LINK_UART_INSTANCE NULL

/* Resource LED definition */
#define HTTP_LED_GPIO_PORT      LED2_GPIO_Port
#define HTTP_LED_PIN            LED2_Pin
#define COAP_LED_GPIO_PORT      LED2_GPIO_Port
#define COAP_LED_PIN            LED2_Pin
#define LED_ON                  GPIO_PIN_SET
#define LED_OFF                 GPIO_PIN_RESET

/* Flash configuration   */
#define FLASH_LAST_PAGE_ADDR     ((uint32_t)0x080ff800) /* Base @ of Page 511, 2 Kbytes */
#define FLASH_LAST_PAGE_NUMBER     511

/* PPPOSIF CLIENT LED definition */
#define PPPOSIF_CLIENT_LED_GPIO_PORT    LED2_GPIO_Port
#define PPPOSIF_CLIENT_LED_PIN          LED2_Pin

/* DEBUG INTERFACE CONFIGURATION */
#define TRACE_INTERFACE_UART_HANDLE     huart1
#define TRACE_INTERFACE_INSTANCE        USART1

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef       huart4;
extern UART_HandleTypeDef       huart1;

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
