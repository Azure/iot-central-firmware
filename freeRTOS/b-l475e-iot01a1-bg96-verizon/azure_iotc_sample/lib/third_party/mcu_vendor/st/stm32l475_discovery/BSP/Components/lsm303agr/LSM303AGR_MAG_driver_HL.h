/**
 ******************************************************************************
 * @file    LSM303AGR_MAG_driver_HL.h
 * @author  MEMS Application Team
 * @version V4.0.0
 * @date    1-May-2017
 * @brief   This file contains definitions for the LSM303AGR_MAG_driver_HL.c firmware driver
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LSM303AGR_MAG_DRIVER_HL_H
#define __LSM303AGR_MAG_DRIVER_HL_H

#ifdef __cplusplus
extern "C" {
#endif



/* Includes ------------------------------------------------------------------*/
#include "magnetometer.h"

/* Include magnetic sensor component drivers. */
#include "LSM303AGR_MAG_driver.h"
#include "LSM303AGR_Combo_driver_HL.h"


/** @addtogroup BSP BSP
 * @{
 */

/** @addtogroup COMPONENTS COMPONENTS
 * @{
 */

/** @addtogroup LSM303AGR_MAG LSM303AGR_MAG
 * @{
 */

/** @addtogroup LSM303AGR_MAG_Public_Constants Public constants
 * @{
 */

/** @addtogroup LSM303AGR_MAG_SENSITIVITY Sensitivity values based on selected full scale
 * @{
 */

#define LSM303AGR_MAG_SENSITIVITY_FOR_FS_50G  1.5  /**< Sensitivity value for 16 gauss full scale [mgauss/LSB] */

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup LSM303AGR_MAG_Public_Types LSM303AGR_MAG Public Types
 * @{
 */

/**
 * @brief LSM303AGR_MAG magneto specific data internal structure definition
 */

typedef struct
{
  LSM303AGR_Combo_Data_t *comboData;       /* Combo data to manage in software SPI 3-Wire initialization of the combo sensors */
} LSM303AGR_M_Data_t;

/**
 * @}
 */

/** @addtogroup LSM303AGR_MAG_Public_Variables Public variables
 * @{
 */

extern MAGNETO_Drv_t LSM303AGR_M_Drv;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __LSM303AGR_MAG_DRIVER_HL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
