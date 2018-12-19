// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "winsock2.h"

#ifdef USE_OPENSSL
#include "azure_c_shared_utility/tlsio_openssl.h"
#endif
#if USE_CYCLONESSL
#include "azure_c_shared_utility/tlsio_cyclonessl.h"
#endif
#if USE_WOLFSSL
#include "azure_c_shared_utility/tlsio_wolfssl.h"
#endif

#include "azure_c_shared_utility/tlsio_schannel.h"

int platform_init(void)
{
    int result;

    WSADATA wsaData;
    int error_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error_code != 0)
    {
        LogError("WSAStartup failed: 0x%x", error_code);
        result = __FAILURE__;
    }
    else
    {
#ifdef USE_OPENSSL
        tlsio_openssl_init();
#endif
        result = 0;
    }
    return result;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
#ifdef USE_OPENSSL
    return tlsio_openssl_get_interface_description();
#elif USE_CYCLONESSL
    return tlsio_cyclonessl_get_interface_description();
#elif USE_WOLFSSL
    return tlsio_wolfssl_get_interface_description();
#else
#ifndef WINCE
    return tlsio_schannel_get_interface_description();
#else
    LogError("TLS IO interface currently not supported on WEC 2013");
    return (IO_INTERFACE_DESCRIPTION*)NULL;
#endif
#endif
}

STRING_HANDLE platform_get_platform_info(void)
{
    // Expected format: "(<runtime name>; <operating system name>; <platform>)"

    STRING_HANDLE result;
#ifndef WINCE
    SYSTEM_INFO sys_info;
    OSVERSIONINFO osvi;
    char *arch;
    GetSystemInfo(&sys_info);

    switch (sys_info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            arch = "x64";
            break;

        case PROCESSOR_ARCHITECTURE_ARM:
            arch = "ARM";
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            arch = "IA64";
            break;

        case PROCESSOR_ARCHITECTURE_INTEL:
            arch = "x32";
            break;

        default:
            arch = "UNKNOWN";
            break;
    }

    result = NULL;
    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#pragma warning(disable:4996)
    if (GetVersionEx(&osvi))
    {
        DWORD product_type;
        if (GetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &product_type))
        {
            result = STRING_construct_sprintf("(native; WindowsProduct:0x%08x %d.%d; %s)", product_type, osvi.dwMajorVersion, osvi.dwMinorVersion, arch);
        }
    }

    if (result == NULL)
    {
        DWORD dwVersion = GetVersion();
        result = STRING_construct_sprintf("(native; WindowsProduct:Windows NT %d.%d; %s)", LOBYTE(LOWORD(dwVersion)), HIBYTE(LOWORD(dwVersion)), arch);
    }
#pragma warning(default:4996)

#else
    result = STRING_construct("(native; Windows CE; undefined)");
#endif
    if (result == NULL)
    {
        LogError("STRING_construct_sprintf failed");
    }
    return result;
}

void platform_deinit(void)
{
    (void)WSACleanup();

#ifdef USE_OPENSSL
    tlsio_openssl_deinit();
#endif
}
