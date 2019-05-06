/*
 * Copyright (C) 2018, Verizon, Inc. All rights reserved.
 */

/**
 * @file aws_BG96_modem.h
 * @brief BG96 modem interface.
 */

#ifndef _AWS_BG96_MODEM_H_
#define _AWS_BG96_MODEM_H_

#include <stdint.h>
#include "cmsis_os.h"

/**
 * @brief Initialize the BG96 HAL.
 *
 * This function initializes the low level hardware drivers and must be called
 * before calling any other modem API
 *
 */

void BG96_HAL_Init(void);

/**
 * @brief Initialize the BG96 drivers.
 *
 * This function initializes the low level drivers and must be called
 * before calling BG96_Modem_Start
 *
 */

void BG96_Modem_Init(void);

/**
 * @brief Starts the BG96 drivers.
 *
 * This function starts the low level  drivers and must be called last in the sequence
 * e.g. after BG96_HAL_Init and BG96_Modem_Init
 *
 */

void BG96_Modem_Start (void);

#endif /* __AWS_BG96_MODEM_H_ */
