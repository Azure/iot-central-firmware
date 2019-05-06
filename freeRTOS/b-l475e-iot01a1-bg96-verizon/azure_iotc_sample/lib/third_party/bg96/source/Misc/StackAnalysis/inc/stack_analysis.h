/**
  ******************************************************************************
  * @file    stack_analysis.h
  * @author  MCD Application Team
  * @brief   Header for stack_analysis.c module
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
#ifndef STACK_ANALYSIS_H
#define STACK_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  stackAnalysis_false = 0U,
  stackAnalysis_true  = 1U
} stackAnalysis_Bool;

/* Exported constants --------------------------------------------------------*/

/* Number of tasks in the Project :
   to analyse stack size a temporarly buffer will be allocated */
#ifdef THREAD_NUMBER
#define STACK_ANALYSIS_TASK_MAX THREAD_NUMBER
#else
#define STACK_ANALYSIS_TASK_MAX 20
#endif

/* Task name length : used for the trace convenience */
#define STACK_ANALYSIS_TASK_NAME_MAX 15
/* Possible filtering of the stack analysis if force parameter in stackAnalysis_trace
  is set to false
  Trace only total free heap or current free heap of a task is less than :
  TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT % of the starting value
  TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT % of the starting value
  Thread not referenced with addStackSizeByHandle or addStackSizeByName
  interfaces are always traced */
#define TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT  10U
#define TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT 20U
/* Trace on change of free heap
   Thread not referenced with addStackSizeByHandle or addStackSizeByName
   interfaces are always traced */
#define TRACE_ON_CHANGE 1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/*
 Features functions
*/
/* After each TaskCreate call addStackSizeByHandle or addStackSizeByName
  to provide the stackSizeAtCreation to StackAnalysis */
/* Input:
TaskHandle          : Task handle
stackSizeAtCreation : stack size parameter at creation */
/* Output:
true : task has been added in the trace
false: task can't be added because number of tasks
       is higher than STACK_ANALYSIS_TASK_MAX */
stackAnalysis_Bool stackAnalysis_addStackSizeByHandle(
  void *TaskHandle,
  uint16_t stackSizeAtCreation);

/* Input:
TaskName          : Task name (in some cases Task handle is unknown)
stackSizeAtCreation : stack size parameter at creation */
/* Output:
true : task has been added in the trace
false: task can't be added because number of tasks
       is higher than STACK_ANALYSIS_TASK_MAX */
stackAnalysis_Bool stackAnalysis_addStackSizeByName(
  char *TaskName,
  uint16_t stackSizeAtCreation);

/* Use printf interface to manage the output */
/* Input:
force : true trace is done whatever the value of
           TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT
        or TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT;
        useful to have a first footprint
        false trace is done according to parameter
           TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT
        or TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT
        Thread whose stack size has not be referenced by a call
        to addStackSizeByHandle or addStackSizeByName are always traced
*/
/* Output:
true:  stack analysis can be done
false: stack analysis can't be done because number of created tasks
       is higher than STACK_ANALYSIS_TASK_MAX */
stackAnalysis_Bool stackAnalysis_trace(stackAnalysis_Bool force);

#ifdef __cplusplus
}
#endif

#endif /* STACK_ANALYSIS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
