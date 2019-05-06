/**
  ******************************************************************************
  * @file    plf_modem_config.h
  * @author  MCD Application Team
  * @brief   This file contains the modem configuration for BG96
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
#ifndef PLF_MODEM_CONFIG_H
#define PLF_MODEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#if defined(HWREF_AKORIOT_BG96)
/* explicitly defined: using BG96 AkorIot board */
#elif defined(HWREF_B_CELL_BG96_V2)
/* explicitly defined: using BG96 B_CELL_V2 board */
#else
/* default: no board specified -> using BG96 B_CELL_V2 board */
#define HWREF_B_CELL_BG96_V2
#endif

#if defined(HWREF_AKORIOT_BG96)
#define USE_MODEM_BG96
#define CONFIG_MODEM_UART_BAUDRATE (115200U)
#define CONFIG_MODEM_UART_RTS_CTS  (0)
#define CONFIG_MODEM_USE_ARDUINO_CONNECTOR

#else /* default case is HWREF_B_CELL_BG96_V2 */
#define USE_MODEM_BG96
#define CONFIG_MODEM_UART_BAUDRATE (115200U)
#define CONFIG_MODEM_UART_RTS_CTS  (0)
#define CONFIG_MODEM_USE_STMOD_CONNECTOR
#endif /* modem board choice */

/* PDP context parameters (for AT+CGDONT) */
#define PDP_CONTEXT_DEFAULT_MODEM_CID         ((uint8_t) 1U)   /* CID numeric value */
#define PDP_CONTEXT_DEFAULT_MODEM_CID_STRING  "1"  /* CID string value */
#define PDP_CONTEXT_DEFAULT_TYPE              "IP" /* defined in project config files */
#define PDP_CONTEXT_DEFAULT_APN               ""   /* defined in project config files */

/* Pre-defined BG96 band configs(!!! flags are exclusive)
*  if no bands flag is set, we use actual modem settings without modification
*/
#define BG96_BAND_CONFIG_CAT_M1_ONLY_FULL_BANDS   (0)
#define BG96_BAND_CONFIG_CAT_NB1_ONLY_FULL_BANDS  (0)
#define BG96_BAND_CONFIG_GSM_ONLY_FULL_BANDS      (0)
#define BG96_BAND_CONFIG_DEFAULT                  (0)
/* Custom BG96 band configs (!!! pre-defined configs above have to be commented if using custom config) */
#define BG96_BAND_CONFIG_CUSTOM                   (0) /* customizable by user (modify config below */
#define BG96_BAND_CONFIG_VERIZON                  (0) /* pre-defined VERIZON config (M1 only, band 4 + band 13) */
#define BG96_BAND_CONFIG_VODAFONE                 (0) /* pre-defined VODAFONE config (NB1 only, band 8 + band 20) */
#define BG96_BAND_CONFIG_MOne                     (0) /* pre-defined M1 (sgp) config (NB1 only, band 8) */
#define BG96_BAND_CONFIG_Orange                   (0) /* pre-defined Orange (Fr) config (M1 only, band 3 & band 20) */

#if (BG96_BAND_CONFIG_CUSTOM == 1)
/* --- start user customization --- */
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_B20)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_B20)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseqAuto)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
/* --- end user customization --- */
#elif (BG96_BAND_CONFIG_VERIZON == 1)
/* pre-defined VERIZON config - do not modify */
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  (_QCFGbandCatM1_B4 | _QCFGbandCatM1_B13))
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_M1_NB1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
#elif (BG96_BAND_CONFIG_VODAFONE == 1)
/* pre-defined VODAFONE config - do not modify */
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) (_QCFGbandCatNB1_B8 | _QCFGbandCatNB1_B20))
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatNB1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_NB1_M1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
#elif (BG96_BAND_CONFIG_MOne == 1)
/* pre-defined M1 (sgp) config - do not modify */
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_B8)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatNB1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_NB1_M1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
#elif (BG96_BAND_CONFIG_Orange == 1)
/* pre-defined Orange (Fr) config - do not modify */
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  (_QCFGbandCatM1_B3 | _QCFGbandCatM1_B20))
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_M1_NB1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
/* pre-defined configs */
#elif (BG96_BAND_CONFIG_CAT_M1_ONLY_FULL_BANDS == 1)
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_M1_NB1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeLTEOnly)
#elif (BG96_BAND_CONFIG_CAT_NB1_ONLY_FULL_BANDS == 1)
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatNB1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_NB1_M1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeLTEOnly)
#elif (BG96_BAND_CONFIG_GSM_ONLY_FULL_BANDS == 1)
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_GSM_M1_NB1)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeGSMOnly)
#elif (BG96_BAND_CONFIG_DEFAULT == 1)
#define BG96_SET_BANDS    (1)
#define BG96_BAND_GSM     ((ATCustom_BG96_QCFGbandGSM_t)    _QCFGbandGSM_any)
#define BG96_BAND_CAT_M1  ((ATCustom_BG96_QCFGbandCatM1_t)  _QCFGbandCatM1_any)
#define BG96_BAND_CAT_NB1 ((ATCustom_BG96_QCFGbandCatNB1_t) _QCFGbandCatNB1_any)
#define BG96_IOTOPMODE    ((ATCustom_BG96_QCFGiotopmode_t)  _QCFGiotopmodeCatM1CatNB1)
#define BG96_SCANSEQ      ((ATCustom_BG96_QCFGscanseq_t)    _QCFGscanseq_M1_NB1_GSM)
#define BG96_SCANMODE     ((ATCustom_BG96_QCFGscanmode_t)   _QCFGscanmodeAuto)
#else
/* IF FALL HERE, MODEM BAND CONFIG NOT MODIFIED */
#define BG96_SET_BANDS    (0)
#endif

/* Power saving mode settings (PSM)
*  should we send AT+CPSMS write command ?
*/
#define BG96_SEND_READ_CPSMS        (0)
#define BG96_SEND_WRITE_CPSMS       (0)
#if (BG96_SEND_WRITE_CPSMS == 1)
#define BG96_ENABLE_PSM             (1) /* 1 if enabled, 0 if disabled */
#define BG96_CPSMS_REQ_PERIODIC_TAU ("00000100")  /* refer to AT commands reference manual v2.0 to set timer values */
#define BG96_CPSMS_REQ_ACTIVE_TIME  ("00001111")  /* refer to AT commands reference manual v2.0 to set timer values */
#else
#define BG96_ENABLE_PSM             (0)
#endif /* BG96_SEND_WRITE_CPSMS */

/* e-I-DRX setting (extended idle mode DRX)
* should we send AT+CEDRXS write command ?
*/
#define BG96_SEND_READ_CEDRXS       (0)
#define BG96_SEND_WRITE_CEDRXS      (0)
#if (BG96_SEND_WRITE_CEDRXS == 1)
#define BG96_ENABLE_E_I_DRX         (1) /* 1 if enabled, 0 if disabled */
#define BG96_CEDRXS_ACT_TYPE        (5) /* 1 for Cat.M1, 5 for Cat.NB1 */
#else
#define BG96_ENABLE_E_I_DRX         (0)
#endif /* BG96_SEND_WRITE_CEDRXS */

/* Network Information */
#define BG96_OPTION_NETWORK_INFO    (1)  /* 1 if enabled, 0 if disabled */

/* Engineering Mode */
#define BG96_OPTION_ENGINEERING_MODE    (0)  /* 1 if enabled, 0 if disabled */

#ifdef __cplusplus
}
#endif

#endif /*_PLF_MODEM_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
