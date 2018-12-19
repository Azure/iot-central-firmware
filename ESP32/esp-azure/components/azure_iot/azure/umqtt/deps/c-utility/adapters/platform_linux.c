// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/xlogging.h"
#ifdef USE_OPENSSL
#include "azure_c_shared_utility/tlsio_openssl.h"
#endif
#if USE_CYCLONESSL
#include "azure_c_shared_utility/tlsio_cyclonessl.h"
#endif
#if USE_WOLFSSL
#include "azure_c_shared_utility/tlsio_wolfssl.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

int platform_init(void)
{
    int result;
#ifdef USE_OPENSSL
    result = tlsio_openssl_init();
#else
    result = 0;
#endif
    return result;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
#if USE_CYCLONESSL
    return tlsio_cyclonessl_get_interface_description();
#elif USE_WOLFSSL
    return tlsio_wolfssl_get_interface_description();
#else
    return tlsio_openssl_get_interface_description();
#endif
}

STRING_HANDLE platform_get_platform_info(void)
{
    // Expected format: "(<runtime name>; <operating system name>; <platform>)"

    STRING_HANDLE result;
    struct utsname nnn;

    if (uname(&nnn) == 0)
    {
        result = STRING_construct_sprintf("(native; %s; %s)", nnn.sysname, nnn.machine);
    }
    else
    {
        LogInfo("WARNING: failed to find machine info.");
        result = STRING_construct("(native; Linux; undefined)");
    }

    return result;
}

void platform_deinit(void)
{
#ifdef USE_OPENSSL
    tlsio_openssl_deinit();
#endif
}
