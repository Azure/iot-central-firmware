// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#ifdef WIN32
#include <Winsock2.h>
#else // WIN32
#include <arpa/inet.h>
#include <unistd.h>
#endif // WIN32

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_utpm_c/gbfiledescript.h"

#include "azure_utpm_c/tpm_comm.h"
#include "azure_utpm_c/tpm_socket_comm.h"

static const char* const TPM_DEVICE_NAME = "/dev/tpm0";
static const char* const TPM_RM_DEVICE_NAME = "/dev/tpmrm0";

static const char* const TPM_OLD_USERMODE_RESOURCE_MGR_64 = "/usr/lib/x86_64-linux-gnu/libtctisocket.so.0";
static const char* const TPM_OLD_USERMODE_RESOURCE_MGR_32 = "/usr/lib/i386-linux-gnu/libtctisocket.so.0";
static const char* const TPM_OLD_USERMODE_RESOURCE_MGR_ARM = "/usr/lib/arm-linux-gnueabihf/libtctisocket.so.0";
static const char* const TPM_NEW_USERMODE_RESOURCE_MGR_64 = "/usr/lib/x86_64-linux-gnu/libtcti-socket.so.0";
static const char* const TPM_NEW_USERMODE_RESOURCE_MGR_32 = "/usr/lib/i386-linux-gnu/libtcti-socket.so.0";
static const char* const TPM_NEW_USERMODE_RESOURCE_MGR_ARM = "/usr/lib/arm-linux-gnueabihf/libtcti-socket.so.0";

#define MIN_TPM_RESPONSE_LENGTH     10

#define TPM_UM_RM_PORT              2323

#define REMOTE_SEND_COMMAND         8
#define REMOTE_SESSION_END_CMD      20

static const char* const TPM_UM_RM_ADDRESS = "127.0.0.1";

typedef enum
{
    TCI_NONE = 0,
    TCI_SYS_DEV = 1,
    TCI_TRM = 2,
    TCI_OLD_UM_TRM = 4,
} TPM_CONN_INFO;

typedef struct TPM_COMM_INFO_TAG
{
    uint32_t            timeout_value;
    TPM_CONN_INFO       conn_info;
    union
    {
        int                 tpm_device;
        TPM_SOCKET_HANDLE   socket_conn;
    } dev_info;
} TPM_COMM_INFO;

static int write_data_to_tpm(TPM_COMM_INFO* tpm_info, const unsigned char* tpm_bytes, uint32_t bytes_len)
{
    int result;
    int resp_len = write(tpm_info->dev_info.tpm_device, tpm_bytes, bytes_len);
    if (resp_len != (int)bytes_len)
    {
        LogError("Failure writing data to tpm: %d:%s.", errno, strerror(errno));
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int read_data_from_tpm(TPM_COMM_INFO* tpm_info, unsigned char* tpm_bytes, uint32_t* bytes_len)
{
    int result;
    int len_read = read(tpm_info->dev_info.tpm_device, tpm_bytes, *bytes_len);
    if (len_read < MIN_TPM_RESPONSE_LENGTH)
    {
        LogError("Failure reading data from tpm: len: %d - %d:%s.", len_read, errno, strerror(errno));
        result = __FAILURE__;
    }
    else
    {
        *bytes_len = len_read;
        result = 0;
    }
    return result;
}

static int read_sync_bytes(TPM_COMM_INFO* comm_info, unsigned char* tpm_bytes, uint32_t* bytes_len)
{
    return tpm_socket_read(comm_info->dev_info.socket_conn, tpm_bytes, *bytes_len);
}

static int read_sync_cmd(TPM_COMM_INFO* tpm_comm_info, uint32_t* tpm_bytes)
{
    int result;
    uint32_t bytes_len = sizeof(uint32_t);
    result = read_sync_bytes(tpm_comm_info, (unsigned char*)tpm_bytes, &bytes_len);
    if (result == 0)
    {
        int j = htonl(*tpm_bytes);
        *tpm_bytes = j;
    }
    return result;
}

static bool is_ack_ok(TPM_COMM_INFO* tpm_comm_info)
{
    uint32_t end_tag;
    return (read_sync_cmd(tpm_comm_info, &end_tag) == 0 && end_tag == 0);
}

static int send_sync_bytes(TPM_COMM_INFO* comm_info, const unsigned char* cmd_val, size_t byte_len)
{
    return tpm_socket_send(comm_info->dev_info.socket_conn, cmd_val, (uint32_t)byte_len);
}

static int send_sync_cmd(TPM_COMM_INFO* tpm_comm_info, uint32_t cmd_val)
{
    uint32_t net_bytes = htonl(cmd_val);
    return send_sync_bytes(tpm_comm_info, (const unsigned char*)&net_bytes, sizeof(uint32_t));
}

static void close_simulator(TPM_COMM_INFO* tpm_comm_info)
{
    (void)send_sync_cmd(tpm_comm_info, REMOTE_SESSION_END_CMD);
}

static int tpm_usermode_resmgr_connect(TPM_COMM_INFO* handle)
{
    bool result;
    bool oldTrm = access(TPM_OLD_USERMODE_RESOURCE_MGR_64, F_OK) != -1
               || access(TPM_OLD_USERMODE_RESOURCE_MGR_32, F_OK) != -1
               || access(TPM_OLD_USERMODE_RESOURCE_MGR_ARM, F_OK) != -1;
    bool newTrm = access(TPM_NEW_USERMODE_RESOURCE_MGR_64, F_OK) != -1
               || access(TPM_NEW_USERMODE_RESOURCE_MGR_32, F_OK) != -1
               || access(TPM_NEW_USERMODE_RESOURCE_MGR_ARM, F_OK) != -1;
    if (!(oldTrm || newTrm))
    {
        LogError("Failure: No user mode TRM found.");
        result = __FAILURE__;
    }
    else if ((handle->dev_info.socket_conn = tpm_socket_create(TPM_UM_RM_ADDRESS, TPM_UM_RM_PORT)) == NULL)
    {
        LogError("Failure: connecting to user mode TRM.");
        result = __FAILURE__;
    }
    else
    {
        handle->conn_info = TCI_TRM | (oldTrm ? TCI_OLD_UM_TRM : 0);
        result = 0;
    }
    return result;
}

static int send_old_um_trm_data(TPM_COMM_HANDLE handle)
{
    int result;
    unsigned char debugMsgLevel = 0, commandSent = 1;
    if ((handle->conn_info & TCI_OLD_UM_TRM) == 0)
    {
        // This is not an old TRM. No additional data are expected.
        result = 0;
    }
    else if (send_sync_bytes(handle, (const unsigned char*)&debugMsgLevel, 1) != 0)
    {
        LogError("Failure setting debugMsgLevel to TRM");
        result = __FAILURE__;
    }
    else if (send_sync_bytes(handle, (const unsigned char*)&commandSent, 1) != 0)
    {
        LogError("Failure setting commandSent status to TRM");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

TPM_COMM_HANDLE tpm_comm_create(const char* endpoint)
{
    TPM_COMM_INFO* result;
    (void)endpoint;
    if ((result = malloc(sizeof(TPM_COMM_INFO))) == NULL)
    {
        LogError("Failure: malloc tpm communication info.");
    }
    else
    {
        memset(result, 0, sizeof(TPM_COMM_INFO));
        // First check if kernel mode TPM Resource Manager is available
        if ((result->dev_info.tpm_device = open(TPM_RM_DEVICE_NAME, O_RDWR)) >= 0)
        {
            result->conn_info = TCI_SYS_DEV | TCI_TRM;
        }
        // If not, connect to the raw TPM device
        else if ((result->dev_info.tpm_device = open(TPM_DEVICE_NAME, O_RDWR)) >= 0)
        {
            result->conn_info = TCI_SYS_DEV;
        }
        // If the system TPM device is unavalable, try connecting to the user mode TPM resource manager
        else if (tpm_usermode_resmgr_connect(result) != 0)
        {
            LogError("Failure: connecting to the TPM device");
            free(result);
            result = NULL;
        }
    }
    return result;
}

void tpm_comm_destroy(TPM_COMM_HANDLE handle)
{
    if (handle)
    {
        if (handle->conn_info & TCI_SYS_DEV)
        {
            (void)close(handle->dev_info.tpm_device);
        }
        else if (handle->conn_info & TCI_TRM)
        {
            close_simulator(handle);
            tpm_socket_destroy(handle->dev_info.socket_conn);
        }
        free(handle);
    }
}

TPM_COMM_TYPE tpm_comm_get_type(TPM_COMM_HANDLE handle)
{
    (void)handle;
    return TPM_COMM_TYPE_LINUX;
}

int tpm_comm_submit_command(TPM_COMM_HANDLE handle, const unsigned char* cmd_bytes, uint32_t bytes_len, unsigned char* response, uint32_t* resp_len)
{
    int result;
    if (handle == NULL || cmd_bytes == NULL || response == NULL || resp_len == NULL)
    {
        LogError("Invalid argument specified handle: %p, cmd_bytes: %p, response: %p, resp_len: %p.", handle, cmd_bytes, response, resp_len);
        result = __FAILURE__;
    }
    else if (handle->conn_info & TCI_SYS_DEV)
    {
        // Send to TPM
        if (write_data_to_tpm(handle, (const unsigned char*)cmd_bytes, bytes_len) != 0)
        {
            LogError("Failure setting locality to TPM");
            result = __FAILURE__;
        }
        else
        {
            if (read_data_from_tpm(handle, response, resp_len) != 0)
            {
                LogError("Failure reading bytes from tpm");
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    else if (handle->conn_info & TCI_TRM)
    {
        unsigned char locality = 0;
        if (send_sync_cmd(handle, REMOTE_SEND_COMMAND) != 0)
        {
            LogError("Failure preparing sending Remote Command");
            result = __FAILURE__;
        }
        else if (send_sync_bytes(handle, (const unsigned char*)&locality, 1) != 0)
        {
            LogError("Failure setting locality to TPM");
            result = __FAILURE__;
        }
        else if (send_old_um_trm_data(handle) != 0)
        {
            LogError("Failure communicating with old user mode TPM");
            result = __FAILURE__;
        }
        else if (send_sync_cmd(handle, bytes_len) != 0)
        {
            LogError("Failure writing command bit to tpm");
            result = __FAILURE__;
        }
        else if (send_sync_bytes(handle, cmd_bytes, bytes_len))
        {
            LogError("Failure writing data to tpm");
            result = __FAILURE__;
        }
        else
        {
            uint32_t length_byte;

            if (read_sync_cmd(handle, &length_byte) != 0)
            {
                LogError("Failure reading length data from tpm");
                result = __FAILURE__;
            }
            else if (length_byte > *resp_len)
            {
                LogError("Bytes read are greater then bytes expected len_bytes:%u expected: %u", length_byte, *resp_len);
                result = __FAILURE__;
            }
            else
            {
                *resp_len = length_byte;
                if (read_sync_bytes(handle, response, &length_byte) != 0)
                {
                    LogError("Failure reading bytes");
                    result = __FAILURE__;
                }
                else
                {
                    // check the Ack
                    if ( !is_ack_ok(handle) )
                    {
                        LogError("Failure reading TRM ack");
                        result = __FAILURE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }
    else
    {
        LogError("Submitting command to an uninitialized TPM_COMM_HANDLE");
        result = __FAILURE__;
    }
    return result;
}
