/**
  ******************************************************************************
  * @file    at_custom_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          BG96 Quectel modem: LTE-cat-M1 or LTE-cat.NB1(=NB-IOT) or GSM
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
#include "at_custom_modem_api.h"
#include "at_custom_modem_specific.h"
#include "sysctrl_specific.h"
#include "plf_config.h"

/* BG96 COMPILATION FLAGS to define in project option if needed:
*
*  - CHECK_SIM_PIN : if defined, check if PIN code is required in SID_CS_INIT_MODEM
*/

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P1, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("BG96:" format "\n\r", ## args);
#define PrintDBG(format, args...)   printf("BG96:" format "\n\r", ## args);
#define PrintAPI(format, args...)   printf("BG96 API:" format "\n\r", ## args);
#define PrintErr(format, args...)   printf("BG96 ERROR:" format "\n\r", ## args);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
at_status_t atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs)
{
  at_status_t retval = ATSTATUS_ERROR;

#if defined(USE_MODEM_BG96)
  PrintDBG("Init AT func ptrs for device = MODEM QUECTEL BG96")

  /* init function pointers with BG96 functions */
  funcPtrs->f_init = ATCustom_BG96_init;
  funcPtrs->f_checkEndOfMsgCallback = ATCustom_BG96_checkEndOfMsgCallback;
  funcPtrs->f_getCmd = ATCustom_BG96_getCmd;
  funcPtrs->f_extractElement = ATCustom_BG96_extractElement;
  funcPtrs->f_analyzeCmd = ATCustom_BG96_analyzeCmd;
  funcPtrs->f_analyzeParam = ATCustom_BG96_analyzeParam;
  funcPtrs->f_terminateCmd = ATCustom_BG96_terminateCmd;
  funcPtrs->f_get_rsp = ATCustom_BG96_get_rsp;
  funcPtrs->f_get_urc = ATCustom_BG96_get_urc;
  funcPtrs->f_get_error = ATCustom_BG96_get_error;

  retval = ATSTATUS_OK;
#else
#error AT custom does not match with selected modem
#endif /* USE_MODEM_BG96 */

  return (retval);
}

sysctrl_status_t atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

#if defined(USE_MODEM_BG96)
  PrintDBG("Init SysCtrl func ptrs for device = MODEM QUECTEL BG96")

  /* init function pointers with BG96 functions */
  funcPtrs->f_getDeviceDescriptor = SysCtrl_BG96_getDeviceDescriptor;
  funcPtrs->f_power_on =  SysCtrl_BG96_power_on;
  funcPtrs->f_power_off = SysCtrl_BG96_power_off;
  funcPtrs->f_reset_device = SysCtrl_BG96_reset;

  retval = SCSTATUS_OK;
#else
#error SysCtrl does not match with selected modem
#endif /* USE_MODEM_BG96 */

  return (retval);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
