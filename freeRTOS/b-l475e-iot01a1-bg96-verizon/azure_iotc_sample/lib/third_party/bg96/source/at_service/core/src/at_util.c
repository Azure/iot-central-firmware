/**
  ******************************************************************************
  * @file    at_util.c
  * @author  MCD Application Team
  * @brief   This file provides code for atcore utilities
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
#include "at_util.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
uint32_t ATutil_ipow(uint32_t base, uint16_t exp)
{
  /* implementation of power function */
  uint32_t result = 1U;
  while (exp)
  {
    if ((exp & 1U) != 0U)
    {
      result *= base;
    }
    exp >>= 1;
    base *= base;
  }

  return result;
}

uint32_t ATutil_convertStringToInt(uint8_t *p_string, uint16_t size)
{
  uint16_t idx, nb_digit_ignored = 0U, loop = 0U;
  uint32_t conv_nbr = 0U;

  /* auto-detect if this is an hexa value (format: 0x....) */
  if (p_string[1] == 120U)
  {
    /* check if the value exceed maximum size */
    if (size > 10U)
    {
      /* PrintErr("String to convert exceed maximum size"); */
      return (0u);
    }

    /* hexa value */
    nb_digit_ignored = 2U;
    for (idx = 2U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        /* 0 to 9 */
        loop++;
        conv_nbr = conv_nbr + ((p_string[idx] - 48U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else if ((p_string[idx] >= 97U) && (p_string[idx] <= 102U))
      {
        /* a to f */
        loop++;
        conv_nbr = conv_nbr + ((p_string[idx] - 97U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else if ((p_string[idx] >= 65U) && (p_string[idx] <= 70U))
      {
        /* A to F */
        loop++;
        conv_nbr = conv_nbr + ((p_string[idx] - 65U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }
  else
  {
    /* decimal value */
    for (idx = 0U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        loop++;
        conv_nbr = conv_nbr + ((p_string[idx] - 48U) * ATutil_ipow(10U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }

  return (conv_nbr);
}

uint64_t ATutil_convertStringToInt64(uint8_t *p_string, uint16_t size)
{
  uint16_t idx, nb_digit_ignored = 0U, loop = 0U;
  uint64_t conv_nbr = 0U;

  /* auto-detect if this is an hexa value (format: 0x....) */
  if (p_string[1] == 120U)
  {
    /* check if the value exceed maximum size */
    if (size > 18U)
    {
      /* PrintErr("String to convert exceed maximum size"); */
      return (0U);
    }

    /* hexa value */
    nb_digit_ignored = 2U;
    for (idx = 2U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        /* 0 to 9 */
        loop++;
        conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 48U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));

      }
      else if ((p_string[idx] >= 97U) && (p_string[idx] <= 102U))
      {
        /* a to f */
        loop++;
        conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 97U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else if ((p_string[idx] >= 65U) && (p_string[idx] <= 70U))
      {
        /* A to F */
        loop++;
        conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 65U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }

    }
  }
  else
  {
    /* decimal value */
    for (idx = 0U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        loop++;
        conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 48U) * ATutil_ipow(10U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }

  return (conv_nbr);
}

uint64_t ATutil_convertHexaStringToInt64(uint8_t *p_string, uint16_t size)
{
  uint16_t idx, nb_digit_ignored = 0U, loop = 0U;
  uint64_t conv_nbr = 0U;
  uint16_t max_str_size;

  /* this function assumes that the string value is an hexadecimal value with or without Ox prefix */

  /* auto-detect if 0x is present */
  if (p_string[1] == 120U)
  {
    /* 0x is present */
    max_str_size = 18U;
    nb_digit_ignored = 2U;
  }
  else
  {
    /* 0x is not present */
    max_str_size = 16U;
    nb_digit_ignored = 0U;
  }


  /* check if the value exceed maximum size */
  if (size > max_str_size)
  {
    /* PrintErr("String to convert exceed maximum size %d", max_str_size); */
    return (0U);
  }

  /* hexa value */
  for (idx = nb_digit_ignored; idx < size; idx++)
  {
    /* consider only the numbers */
    if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
    {
      /* 0 to 9 */
      loop++;
      conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 48U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else if ((p_string[idx] >= 97U) && (p_string[idx] <= 102U))
    {
      /* a to f */
      loop++;
      conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 97U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else if ((p_string[idx] >= 65U) && (p_string[idx] <= 70U))
    {
      /* A to F */
      loop++;
      conv_nbr = conv_nbr + (((uint64_t)p_string[idx] - 65U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else
    {
      nb_digit_ignored++;
    }

  }

  return (conv_nbr);
}

void ATutil_convertStringToUpperCase(uint8_t *p_string, uint16_t size)
{
  uint16_t idx = 0U;
  while ((p_string[idx] != 0U) && (idx < size))
  {
    /* if lower case character... */
    if ((p_string[idx] >= 97U) && (p_string[idx] <= 122U))
    {
      /* ...convert it to uppercase character */
      p_string[idx] -= 32U;
    }
    idx++;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
