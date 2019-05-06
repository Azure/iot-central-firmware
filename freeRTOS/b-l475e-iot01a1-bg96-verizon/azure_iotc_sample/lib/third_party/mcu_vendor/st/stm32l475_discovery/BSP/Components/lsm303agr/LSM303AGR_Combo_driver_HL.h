/**
 ******************************************************************************
 * @file    LSM303AGR_Combo_driver_HL.h
 * @author  MEMS Application Team
 * @version V3.0.0
 * @date    27-January-2016
 * @brief   This file contains definitions for the LSM303AGR combo driver
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
#ifndef __LSM303AGR_COMBO_DRIVER_HL_H
#define __LSM303AGR_COMBO_DRIVER_HL_H

#ifdef __cplusplus
extern "C" {
#endif


/** @addtogroup BSP BSP
 * @{
 */

/** @addtogroup COMPONENTS COMPONENTS
 * @{
 */

/** @addtogroup LSM303AGR_COMBO LSM303AGR_COMBO
 * @{
 */

/** @addtogroup LSM303AGR_COMBO_Public_Constants Public constants
 * @{
 */
#define LSM303AGR_SENSORS_MAX_NUM  1     /**< LSM303AGR max number of instances */

/**
 * @}
 */

/** @addtogroup LSM303AGR_COMBO_Public_Types LSM303AGR_COMBO Public Types
 * @{
 */

/**
 * @brief LSM303AGR combo specific data internal structure definition
 */
typedef struct
{
  uint8_t isAccInitialized;
  uint8_t isMagInitialized;
} LSM303AGR_Combo_Data_t;

/**
 * @}
 */

/** @addtogroup LSM303AGR_COMBO_Public_Variables Public variables
 * @{
 */

extern LSM303AGR_Combo_Data_t LSM303AGR_Combo_Data[LSM303AGR_SENSORS_MAX_NUM];

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

#endif /* __LSM303AGR_COMBO_DRIVER_HL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
