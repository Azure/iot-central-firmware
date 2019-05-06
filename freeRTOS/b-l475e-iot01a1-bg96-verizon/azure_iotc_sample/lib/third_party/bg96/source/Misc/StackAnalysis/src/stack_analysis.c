/**
  ******************************************************************************
  * @file    stack_analysis.c
  * @author  MCD Application Team
  * @brief   This file implements Stack analysis debug facilities
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
#include "stack_analysis.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"


/* Private defines -----------------------------------------------------------*/
#if (STACK_ANALYSIS_TRACE == 1)
#define USE_TRACE_STACK_ANALYSIS      (1)
#else
#define USE_TRACE_STACK_ANALYSIS      (0)
#endif /* STACK_ANALYSIS_TRACE */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_STACK_ANALYSIS == 1)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) \
TracePrint(DBG_CHAN_MAIN, DBL_LVL_P0, "Stack-A:" format "\n\r", ## args)
#define PrintDBG(format, args...)  \
TracePrint(DBG_CHAN_MAIN, DBL_LVL_P1, "Stack-A:" format "\n\r", ## args)
#define PrintERR(format, args...)  \
TracePrint(DBG_CHAN_MAIN, DBL_LVL_ERR, "Stack-A ERROR:" format "\n\r", ## args)
#else
#define PrintINFO(format, args...) printf("Stack-A:" format "\n\r", ## args);
#define PrintDBG(format, args...)  printf("Stack-A:" format "\n\r", ## args);
#define PrintERR(format, args...)  printf("Stack-A ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintERR(format, args...)   do {} while(0);
#endif /* USE_TRACE_STACK_ANALYSIS */

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/* Private typedef -----------------------------------------------------------*/
typedef uint8_t stack_char_t;

typedef struct
{
  void *TaskHandle;
  uint16_t stackSizeAtCreation;
  uint16_t stackSizeFree;
} TaskAnalysis_t;

/* Private define ------------------------------------------------------------*/
#define tskBLOCKED_CHAR   ( 'B' )
#define tskREADY_CHAR     ( 'R' )
#define tskDELETED_CHAR   ( 'D' )
#define tskSUSPENDED_CHAR ( 'S' )
#define tskUNKNOWN_CHAR   ( ' ' )

/* Private variables ---------------------------------------------------------*/
static TaskAnalysis_t TaskAnalysisList[STACK_ANALYSIS_TASK_MAX];
static uint8_t TaskAnalysisListNb = 0U;
/* Initiliaze at the maximum possible value */
static uint16_t GlobalstackSizeFree = (uint16_t)(configTOTAL_HEAP_SIZE);

/* Private function prototypes -----------------------------------------------*/
static stackAnalysis_Bool getStackSizeByHandle(
  TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static stackAnalysis_Bool getStackSizeByName(
  const stack_char_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree);

static void formatTaskName(stack_char_t *pcBuffer,
                           size_t size_max,
                           const stack_char_t *pcTaskName);

/* Private functions ---------------------------------------------------------*/
static stackAnalysis_Bool getStackSizeByHandle(
  TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  stackAnalysis_Bool found;

  i = 0U;
  found = stackAnalysis_false;
  while ((found == stackAnalysis_false)
         && (i < TaskAnalysisListNb))
  {
    if (TaskHandle == TaskAnalysisList[i].TaskHandle)
    {
      found = stackAnalysis_true;
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

static stackAnalysis_Bool getStackSizeByName(
  const stack_char_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  stackAnalysis_Bool found;

  i = 0U;
  found = stackAnalysis_false;

  while ((found == stackAnalysis_false)
         && (i < TaskAnalysisListNb))
  {
    if (strcmp((const char *)TaskName, TaskAnalysisList[i].TaskHandle) == 0)
    {
      found = stackAnalysis_true;
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree)
{
  if (indice < TaskAnalysisListNb)
  {
    TaskAnalysisList[indice].stackSizeFree = stackSizeFree;
  }
}

static void formatTaskName(stack_char_t *pcBuffer,
                           size_t size_max,
                           const stack_char_t *pcTaskName)
{
  size_t i;

  /* Start by copying the entire string. */
  memcpy((char *)pcBuffer,
         (const char *)pcTaskName,
         MIN(size_max - 1U, strlen((const char *)pcTaskName)));

  /* Pad the end of the string with spaces to ensure columns line up
     when printed out. */
  for (i = (size_t)(MIN(size_max - 1U, strlen((const char *)pcTaskName)));
       i < (size_max - 1U);
       i++)
  {
    pcBuffer[i] = ' ';
  }
  /* Terminate. */
  pcBuffer[i] = (char)0x00;
}



/* Public functions ----------------------------------------------------------*/
stackAnalysis_Bool stackAnalysis_addStackSizeByHandle(
  void *TaskHandle,
  uint16_t stackSizeAtCreation)
{
  stackAnalysis_Bool result;

  if (TaskHandle == NULL)
  {
    PrintINFO("Error: NULL TaskHandle passed in stack_analysis function ")
    result = stackAnalysis_false;
  }
  else
  {
    if ((int32_t)TaskAnalysisListNb < STACK_ANALYSIS_TASK_MAX)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskHandle;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      TaskAnalysisListNb++;
      result = stackAnalysis_true;
    }
    else
    {
      PrintINFO("Error: Too many tasks added in stack_analysis %d ",
                STACK_ANALYSIS_TASK_MAX)
      result = stackAnalysis_false;
    }
  }

  return result;
}

stackAnalysis_Bool stackAnalysis_addStackSizeByName(
  char *TaskName,
  uint16_t stackSizeAtCreation)
{
  stackAnalysis_Bool result;

  if (TaskName == NULL)
  {
    PrintINFO("Error: NULL TaskName passed in stack_analysis function ")
    result = stackAnalysis_false;
  }
  else
  {
    if ((int32_t)TaskAnalysisListNb < STACK_ANALYSIS_TASK_MAX)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskName;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      TaskAnalysisListNb++;
      result = stackAnalysis_true;
    }
    else
    {
      PrintINFO("Error: Too many tasks added in stack_analysis %d ",
                STACK_ANALYSIS_TASK_MAX)
      result = stackAnalysis_false;
    }
  }

  return result;
}

stackAnalysis_Bool stackAnalysis_trace(stackAnalysis_Bool force)
{
  stackAnalysis_Bool result, found, printed;
  TaskStatus_t *pxTaskStatusArray;
  UBaseType_t uxArraySize;
  uint16_t i, j;
  uint32_t ulTotalRunTime, ulStatsAsPercentage;
  uint16_t usStackSizeAtCreation, usStackSizeFree;
  stack_char_t xTaskName[STACK_ANALYSIS_TASK_NAME_MAX];
  stack_char_t cTaskStatus;
  size_t freeHeap1, freeHeap2;
  uint8_t indice;

  result = stackAnalysis_false;
  printed = stackAnalysis_false;
  usStackSizeFree = 0U;
  freeHeap2 = 0U;

  uxArraySize = uxTaskGetNumberOfTasks();

  /* Before allocation GetFreeHeapSize() */
  freeHeap1 = xPortGetFreeHeapSize();

  /* Allocate a TaskStatus_t structure for each task. */
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL)
  {
    /*
     * It is recommended that production systems call uxTaskGetSystemState()
     * directly to get access to raw stats data, rather than indirectly
     * through a call to vTaskList().
    */
    /* Generate raw status information about each task. */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
                                       uxArraySize,
                                       &ulTotalRunTime);

    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;

    /* After allocation GetFreeHeapSize() */
    freeHeap2 = xPortGetFreeHeapSize();

    if ((force == stackAnalysis_true)
        || (freeHeap2 < configTOTAL_HEAP_SIZE * TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT / 100U)
        || ((TRACE_ON_CHANGE == 1)
            && (freeHeap2 < GlobalstackSizeFree)))
    {
      printed = stackAnalysis_true;
      PrintINFO("===========================")
      PrintINFO("Free Heap : %ld=>%ld (%ld) ", GlobalstackSizeFree,
                freeHeap2,
                configTOTAL_HEAP_SIZE)
      PrintINFO("(%ld used temporarily for this analysis)", freeHeap1 - freeHeap2)
      GlobalstackSizeFree = (uint16_t)freeHeap2;
    }
    if (force == stackAnalysis_true)
    {
      PrintINFO("Task List (%d): ", uxArraySize)
    }

    /* For each populated position in the pxTaskStatusArray array,
       format the raw data as human readable ASCII data */
    for (i = 0U; i < uxArraySize; i++)
    {
      uint32_t taskNumber;
      found = stackAnalysis_false;
      usStackSizeAtCreation = 0U;
      j = 0U;
      cTaskStatus = tskUNKNOWN_CHAR;

      /* Ordering Task according to their xTaskNumber */
      while ((found == stackAnalysis_false)
             && (j < uxArraySize))
      {
        taskNumber = pxTaskStatusArray[j].xTaskNumber;
        if (taskNumber == i + 1U)
        {
          found = stackAnalysis_true;
        }
        else
        {
          j++;
        }
      }

      if (j < uxArraySize)
      {
        switch (pxTaskStatusArray[j].eCurrentState)
        {
          case eReady:
          {
            cTaskStatus = tskREADY_CHAR;
            break;
          }

          case eBlocked:
          {
            cTaskStatus = tskBLOCKED_CHAR;
            break;
          }

          case eSuspended:
          {
            cTaskStatus = tskSUSPENDED_CHAR;
            break;
          }

          case eDeleted:
          {
            cTaskStatus = tskDELETED_CHAR;
            break;
          }

          default:
          {
            /* Should not get here, but it is included
               to prevent static checking errors. */
            break;
          }
        }

        /* Avoid divide by zero errors. */
        if (ulTotalRunTime > 0U)
        {
          /* What percentage of the total run time has the task used?
             This will always be rounded down to the nearest integer.
             ulTotalRunTimeDiv100 has already been divided by 100. */
          ulStatsAsPercentage = pxTaskStatusArray[j].ulRunTimeCounter / ulTotalRunTime;
        }
        else
        {
          ulStatsAsPercentage = 0U;
        }

        formatTaskName(xTaskName,
                       (uint32_t)STACK_ANALYSIS_TASK_NAME_MAX,
                       (const stack_char_t *)pxTaskStatusArray[j].pcTaskName);
        found = getStackSizeByHandle(pxTaskStatusArray[j].xHandle,
                                     &usStackSizeAtCreation,
                                     &usStackSizeFree,
                                     &indice);
        if (found == stackAnalysis_false)
        {
          found = getStackSizeByName((const stack_char_t *)pxTaskStatusArray[j].pcTaskName,
                                     &usStackSizeAtCreation,
                                     &usStackSizeFree,
                                     &indice);
        }

        if ((force == stackAnalysis_true)
            || (found == stackAnalysis_false)
            || (pxTaskStatusArray[j].usStackHighWaterMark
                < (usStackSizeAtCreation * (uint16_t)TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT / (uint16_t)100))
            || ((TRACE_ON_CHANGE == 1)
                && (pxTaskStatusArray[j].usStackHighWaterMark < usStackSizeFree)))
        {
          if (printed == stackAnalysis_false)
          {
            PrintINFO("===========================")
          }
          if (ulStatsAsPercentage > 0UL)
          {
            printed = stackAnalysis_true;
            PrintINFO("%s  Num:%2u FreeHeap:%4u=>%4u/%4u Prio:%u State:%c Time:%lu  %lu%% ",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus,
                      pxTaskStatusArray[j].ulRunTimeCounter,
                      ulStatsAsPercentage)
            if (found == stackAnalysis_true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
          }
          else
          {
            /* If the percentage is zero here then the task has
               consumed less than 1% of the total run time. */
#if ( configGENERATE_RUN_TIME_STATS == 1 )
            printed = stackAnalysis_true;
            PrintINFO("%s  Num:%2u FreeHeap:%4u=>%4u/%4u Prio:%u State:%c Time:<1%%",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus)
            if (found == stackAnalysis_true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
#else
            printed = stackAnalysis_true;
            PrintINFO("%s  Num:%2u FreeHeap:%4u=>%4u/%4u Prio:%u State:%c ",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus)
            if (found == stackAnalysis_true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
#endif
          }
        }
      }
    }
    if (printed == stackAnalysis_true)
    {
      PrintINFO("===========================")
    }
    /* The array is no longer needed, free the memory it consumes. */
    vPortFree(pxTaskStatusArray);

    result = stackAnalysis_true;
  }
  else
  {
    PrintERR("NOT enough memory for stack_analysis study - task nb: %d ", uxArraySize)
  }

#if (USE_TRACE_STACK_ANALYSIS == 0)
  /* To avoid warning if trace not activated (variables are set but not used) */
  if ((cTaskStatus == tskUNKNOWN_CHAR)
      && (freeHeap1 == 0U))
  {
    i = 0U;
  }
#endif

  return result;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
