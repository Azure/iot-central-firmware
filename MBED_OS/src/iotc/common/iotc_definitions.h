// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOTC_LITE_DEFINITIONS_H
#define AZURE_IOTC_LITE_DEFINITIONS_H

#define AZURE_MQTT_SERVER_PORT  8883
#define AZURE_HTTPS_SERVER_PORT  443

#define IOTC_SERVER_RESPONSE_TIMEOUT 20 // seconds

#define SSL_CLIENT_CERT_PEM  NULL
#define SSL_CLIENT_PRIVATE_KEY_PEM NULL

/* Baltimore */
#define SSL_CA_PEM_DEF  \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n" \
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n" \
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n" \
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n" \
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n" \
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n" \
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n" \
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n" \
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n" \
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n" \
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n" \
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n" \
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n" \
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n" \
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n" \
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n" \
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n" \
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n" \
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n" \
"-----END CERTIFICATE-----\r\n"

// GENERAL
#define TO_STRING_(s) #s
#define TO_STRING(s) TO_STRING_(s)
#define iotc_max(a,b) (a > b ? a : b)
#define iotc_min(a,b) (a < b ? a : b)

#define STRING_BUFFER_16     16
#define STRING_BUFFER_32     32
#define STRING_BUFFER_64     64
#define STRING_BUFFER_128   128
#define STRING_BUFFER_256   256
#define STRING_BUFFER_512   512
#define STRING_BUFFER_1024 1024
#define STRING_BUFFER_4096 4096

#define NTP_SYNC_PERIOD (6 * 60 * 60 * 1000)

#define AZIOTC_API_MAJOR_VERSION 0.
#define AZIOTC_API_MINOR_VERSION 2.
#define AZIOTC_API_PATCH_VERSION 0
#define AZIOTC_API_VERSION         TO_STRING(AZIOTC_API_MAJOR_VERSION \
AZIOTC_API_MINOR_VERSION AZIOTC_API_PATCH_VERSION) "-msiotc"

#define AZURE_IOT_CENTRAL_CLIENT_SIGNATURE "user-agent: iot-central-client/" AZIOTC_API_VERSION

#if defined(_DEBUG) || defined(DEBUG)
#define ASSERT_OR_FAIL_FAST(x) assert(x)
#else // defined(_DEBUG) || defined(DEBUG)
#define ASSERT_OR_FAIL_FAST(x) if (!(x)) { LOG_ERROR(TO_STRING(x) "condition has failed"); }
#endif // defined(_DEBUG) || defined(DEBUG)

#ifdef MBED_STATIC_ASSERT
#define ASSERT_STATIC MBED_STATIC_ASSERT
#else
#define ASSERT_STATIC static_assert
#endif

#endif // AZURE_IOTC_LITE_DEFINITIONS_H