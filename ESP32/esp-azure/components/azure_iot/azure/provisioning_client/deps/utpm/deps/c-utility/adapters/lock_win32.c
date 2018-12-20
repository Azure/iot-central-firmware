// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/lock.h"
#include <windows.h>
#include <stdlib.h>
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/macro_utils.h"

LOCK_HANDLE Lock_Init(void)
{
    /* Codes_SRS_LOCK_10_002: [Lock_Init on success shall return a valid lock handle which should be a non NULL value] */
    /* Codes_SRS_LOCK_10_003: [Lock_Init on error shall return NULL ] */
    HANDLE result = CreateSemaphoreW(NULL, 1, 1, NULL);
    if (result == NULL)
    {
        LogError("CreateSemaphore failed.");
    }

    return (LOCK_HANDLE)result;
}

LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)
{
    LOCK_RESULT result;
    if (handle == NULL)
    {
        /* Codes_SRS_LOCK_10_007: [Lock_Deinit on NULL handle passed returns LOCK_ERROR] */
        LogError("Invalid argument; handle is NULL.");
        result = LOCK_ERROR;
    }
    else
    {
        /* Codes_SRS_LOCK_10_012: [Lock_Deinit frees the memory pointed by handle] */
        CloseHandle((HANDLE)handle);
        result = LOCK_OK;
    }

    return result;
}

LOCK_RESULT Lock(LOCK_HANDLE handle)
{
    LOCK_RESULT result;
    if (handle == NULL)
    {
        /* Codes_SRS_LOCK_10_007: [Lock on NULL handle passed returns LOCK_ERROR] */
        LogError("Invalid argument; handle is NULL.");
        result = LOCK_ERROR;
    }
    else
    {
        DWORD rv = WaitForSingleObject((HANDLE)handle, INFINITE);
        switch (rv)
        {
            case WAIT_OBJECT_0:
                /* Codes_SRS_LOCK_10_005: [Lock on success shall return LOCK_OK] */
                result = LOCK_OK;
                break;
            case WAIT_ABANDONED:
                LogError("WaitForSingleObject returned 'abandoned'.");
                /* Codes_SRS_LOCK_10_006: [Lock on error shall return LOCK_ERROR] */
                result = LOCK_ERROR;
                break;
            case WAIT_TIMEOUT:
                LogError("WaitForSingleObject timed out.");
                /* Codes_SRS_LOCK_10_006: [Lock on error shall return LOCK_ERROR] */
                result = LOCK_ERROR;
                break;
            case WAIT_FAILED:
                LogError("WaitForSingleObject failed: %d", GetLastError());
                /* Codes_SRS_LOCK_10_006: [Lock on error shall return LOCK_ERROR] */
                result = LOCK_ERROR;
                break;
            default:
                LogError("WaitForSingleObject returned an invalid value.");
                /* Codes_SRS_LOCK_10_006: [Lock on error shall return LOCK_ERROR] */
                result = LOCK_ERROR;
                break;
        }
    }

    return result;
}

LOCK_RESULT Unlock(LOCK_HANDLE handle)
{
    LOCK_RESULT result;
    if (handle == NULL)
    {
        /* Codes_SRS_LOCK_10_007: [Unlock on NULL handle passed returns LOCK_ERROR] */
        LogError("Invalid argument; handle is NULL.");
        result = LOCK_ERROR;
    }
    else
    {
        if (ReleaseSemaphore((HANDLE)handle, 1, NULL))
        {
            /* Codes_SRS_LOCK_10_009: [Unlock on success shall return LOCK_OK] */
            result = LOCK_OK;
        }
        else
        {
            /* Codes_SRS_LOCK_10_010: [Unlock on error shall return LOCK_ERROR] */
            LogError("ReleaseSemaphore failed: %d", GetLastError());
            result = LOCK_ERROR;
        }
    }

    return result;
}
