/*
 * Copyright (C) 2018, Verizon, Inc. All rights reserved.
 */

/* Includes ------------------------------------------------------------------*/
#include "bg96_modem.h"
#include "board_buttons.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "plf_config.h"
#include "rtc.h"
#include "usart.h"
#include "dc_common.h"
#include "dc_control.h"
#include "cellular_init.h"
#include "dc_cellular.h"
#include "ipc_uart.h"
#include "com_sockets.h"
#include "nifman.h"
#include "radio_mngt.h"
#include "cellular_service.h"
#include "cellular_service_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cellular_service_task.h"

#define GOOD_PINCODE "1234"
#define BAD_PINCODE  "6789"
#define STACK_ANALYSIS_COUNTER  50  /*  Stack analysed every 5s */


/* Function prototypes -------------------------------------------------------*/

static void BG96_net_up_cb ( dc_com_event_id_t dc_event_id, void* private_gui_data );

#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 15 )

/**
 * @brief Initialize the BG96 HAL.
 *
 * This function initializes the low level hardware drivers and must be called
 * before calling any other modem API
 *
 */

void BG96_HAL_Init(void)
{
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_UART4_Init();
}

/**
 * @brief Initialize the BG96 drivers.
 *
 * This function initializes the low level drivers and must be called
 * before calling BG96_Modem_Start
 *
 */

void BG96_Modem_Init(void) {

    /* Do not change order */

      (void)com_init();

      CST_cellular_service_init();

      cellular_init();

      dc_com_register_gen_event_cb(&dc_com_db, BG96_net_up_cb, (void*) NULL);

}

/**
 * @brief Starts the BG96 drivers.
 *
 * This function starts the low level  drivers and must be called last in the sequence
 * e.g. after BG96_HAL_Init and BG96_Modem_Init
 *
 */

void BG96_Modem_Start ()
{

    cellular_start();
}

/**
 * @brief Callback function.
 *
 * This function is called whenever there is an event on the network interface
 * manager. Only interested when the modem is up.
 *
 */

int is_BG96_online = 0; // down

int isBG96Online() { return is_BG96_online; }

void logCellInfo(const char* msg, int printModemActivation) {
  dc_cellular_info_t info;
  dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&info, sizeof(info));

  if (info.cs_signal_level == 0) {
    if (printModemActivation == 1) {
      configPRINTF(("%s : Waiting for cellular modem activation\r\n", msg));
    }
  } else {
    configPRINTF(("%s : signal level: %d level_db: %d\r\n", msg,
        info.cs_signal_level, info.cs_signal_level_db));
  }
}

static void BG96_net_up_cb ( dc_com_event_id_t dc_event_id, void* private_gui_data )
{
    dc_service_rt_state_t state = DC_SERVICE_FAIL;
    bool unknownEvent = true;
    if( dc_event_id == DC_COM_NIFMAN_INFO)
    {
        dc_nifman_info_t  info;
        dc_com_read( &dc_com_db, DC_COM_NIFMAN_INFO, (void *)&info, sizeof(info));
        state = info.rt_state;
        unknownEvent = false;
    } else if (dc_event_id == DC_COM_CELLULAR_INFO) {
        logCellInfo("DC_COM_CELLULAR_INFO", 0);
    } else if (dc_event_id == DC_COM_CELLULAR_DATA_INFO) {
        logCellInfo("DC_COM_CELLULAR_DATA_INFO", 0);
    } else if (dc_event_id == DC_COM_PPP_CLIENT_INFO) {
        logCellInfo("DC_COM_PPP_CLIENT_INFO", 0);
    } else if (dc_event_id == DC_COM_RADIO_LTE_INFO) {
        logCellInfo("DC_COM_RADIO_LTE_INFO", 0);
    } else if (dc_event_id == DC_COM_NFMC_TEMPO_INFO) {
        logCellInfo("DC_COM_NFMC_TEMPO_INFO", 0);
    } else if (dc_event_id == DC_COM_SIM_INFO) {
        logCellInfo("DC_COM_SIM_INFO", 0);
    } else {
        unknownEvent = true;
    }

    if(state  ==  DC_SERVICE_ON)
    {
        is_BG96_online = 1; // online
        configPRINTF(("\tNetwork is up\r\n"));
    } else if (!unknownEvent) {
        dc_cellular_info_t info;
        dc_com_read( &dc_com_db, DC_COM_CELLULAR_INFO, (void *)&info, sizeof(info));
        // is_BG96_online = 0; // down
        configPRINTF(("\tNetwork is down ? Signal level => %d\r\n", info.cs_signal_level));
    }
}
