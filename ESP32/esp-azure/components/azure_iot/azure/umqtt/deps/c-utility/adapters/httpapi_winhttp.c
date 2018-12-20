// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "windows.h"
#include "winhttp.h"
#include "azure_c_shared_utility/httpapi.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/x509_schannel.h"
#include "azure_c_shared_utility/shared_util_options.h"

DEFINE_ENUM_STRINGS(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES)

typedef enum HTTPAPI_STATE_TAG
{
    HTTPAPI_NOT_INITIALIZED,
    HTTPAPI_INITIALIZED
} HTTPAPI_STATE;

typedef struct HTTP_HANDLE_DATA_TAG
{
    /*working set*/
    HINTERNET ConnectionHandle;
    X509_SCHANNEL_HANDLE x509SchannelHandle;
    /*options*/
    unsigned int timeout;
    const char* x509certificate;
    const char* x509privatekey;
    const char* proxy_host;
    const char* proxy_username;
    const char* proxy_password;
} HTTP_HANDLE_DATA;

static HTTPAPI_STATE g_HTTPAPIState = HTTPAPI_NOT_INITIALIZED;

/*There's a global SessionHandle for all the connections*/
static HINTERNET g_SessionHandle;
static size_t nUsersOfHTTPAPI = 0; /*used for reference counting (a weak one)*/

/*returns NULL if it failed to construct the headers*/
static const char* ConstructHeadersString(HTTP_HEADERS_HANDLE httpHeadersHandle)
{
    char* result;
    size_t headersCount;

    if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount) != HTTP_HEADERS_OK)
    {
        result = NULL;
        LogError("HTTPHeaders_GetHeaderCount failed.");
    }
    else
    {
        size_t i;

        /*the total size of all the headers is given by sumof(lengthof(everyheader)+2)*/
        size_t toAlloc = 0;
        for (i = 0; i < headersCount; i++)
        {
            char *temp;
            if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) == HTTP_HEADERS_OK)
            {
                toAlloc += strlen(temp);
                toAlloc += 2;
                free(temp);
            }
            else
            {
                LogError("HTTPHeaders_GetHeader failed");
                break;
            }
        }

        if (i < headersCount)
        {
            result = NULL;
        }
        else
        {
            result = (char*)malloc(toAlloc*sizeof(char) + 1 );

            if (result == NULL)
            {
                LogError("unable to malloc");
                /*let it be returned*/
            }
            else
            {
                result[0] = '\0';
                for (i = 0; i < headersCount; i++)
                {
                    char* temp;
                    if (HTTPHeaders_GetHeader(httpHeadersHandle, i, &temp) != HTTP_HEADERS_OK)
                    {
                        LogError("unable to HTTPHeaders_GetHeader");
                        break;
                    }
                    else
                    {
                        (void)strcat(result, temp);
                        (void)strcat(result, "\r\n");
                        free(temp);
                    }
                }

                if (i < headersCount)
                {
                    free(result);
                    result = NULL;
                }
                else
                {
                    /*all is good*/
                }
            }
        }
    }

    return result;
}

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    HTTPAPI_RESULT result;

    if (nUsersOfHTTPAPI == 0)
    {
        if ((g_SessionHandle = WinHttpOpen(
            NULL,
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0)) == NULL)
        {
            LogErrorWinHTTPWithGetLastErrorAsString("WinHttpOpen failed.");
            result = HTTPAPI_INIT_FAILED;
        }
        else
        {
            nUsersOfHTTPAPI++;
            g_HTTPAPIState = HTTPAPI_INITIALIZED;
            result = HTTPAPI_OK;
        }
    }
    else
    {
        nUsersOfHTTPAPI++;
        result = HTTPAPI_OK;
    }

    return result;
}

void HTTPAPI_Deinit(void)
{
    if (nUsersOfHTTPAPI > 0)
    {
        nUsersOfHTTPAPI--;
        if (nUsersOfHTTPAPI == 0)
        {
            if (g_SessionHandle != NULL)
            {
                (void)WinHttpCloseHandle(g_SessionHandle);
                g_SessionHandle = NULL;
                g_HTTPAPIState = HTTPAPI_NOT_INITIALIZED;
            }
        }
    }


}

HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    HTTP_HANDLE_DATA* result;
    if (g_HTTPAPIState != HTTPAPI_INITIALIZED)
    {
        LogError("g_HTTPAPIState not HTTPAPI_INITIALIZED");
        result = NULL;
    }
    else
    {
        result = (HTTP_HANDLE_DATA*)malloc(sizeof(HTTP_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc returned NULL.");
        }
        else
        {
            wchar_t* hostNameTemp;
            size_t hostNameTemp_size = MultiByteToWideChar(CP_ACP, 0, hostName, -1, NULL, 0);
            if (hostNameTemp_size == 0)
            {
                LogError("MultiByteToWideChar failed");
                free(result);
                result = NULL;
            }
            else
            {
                hostNameTemp = (wchar_t*)malloc(sizeof(wchar_t)*hostNameTemp_size);
                if (hostNameTemp == NULL)
                {
                    LogError("malloc failed");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (MultiByteToWideChar(CP_ACP, 0, hostName, -1, hostNameTemp, (int)hostNameTemp_size) == 0)
                    {
                        LogError("MultiByteToWideChar failed");
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        result->ConnectionHandle = WinHttpConnect(
                            g_SessionHandle,
                            hostNameTemp,
                            INTERNET_DEFAULT_HTTPS_PORT,
                            0);

                        if (result->ConnectionHandle == NULL)
                        {
                            LogErrorWinHTTPWithGetLastErrorAsString("WinHttpConnect returned NULL.");
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            result->timeout = 60000;
                            result->x509certificate = NULL;
                            result->x509privatekey = NULL;
                            result->x509SchannelHandle = NULL;
                            result->proxy_host = NULL;
                            result->proxy_username = NULL;
                            result->proxy_password = NULL;
                        }
                    }
                    free(hostNameTemp);
                }
            }
        }
    }

    return (HTTP_HANDLE)result;
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    if (g_HTTPAPIState != HTTPAPI_INITIALIZED)
    {
        LogError("g_HTTPAPIState not HTTPAPI_INITIALIZED");
    }
    else
    {
        HTTP_HANDLE_DATA* handleData = (HTTP_HANDLE_DATA*)handle;

        if (handleData != NULL)
        {
            if (handleData->ConnectionHandle != NULL)
            {
                (void)WinHttpCloseHandle(handleData->ConnectionHandle);
                /*no x509 free because the options are owned by httpapiex.*/
                handleData->ConnectionHandle = NULL;
            }
            if (handleData->proxy_host != NULL)
            {
                free((void*)handleData->proxy_host);
            }
            if (handleData->proxy_username != NULL)
            {
                free((void*)handleData->proxy_username);
            }
            if (handleData->proxy_password != NULL)
            {
                free((void*)handleData->proxy_password);
            }
            x509_schannel_destroy(handleData->x509SchannelHandle);
            free(handleData);
        }
    }
}

HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
    HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
    size_t contentLength, unsigned int* statusCode,
    HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    HTTPAPI_RESULT result;
    if (g_HTTPAPIState != HTTPAPI_INITIALIZED)
    {
        LogError("g_HTTPAPIState not HTTPAPI_INITIALIZED");
        result = HTTPAPI_NOT_INIT;
    }
    else
    {
        HTTP_HANDLE_DATA* handleData = (HTTP_HANDLE_DATA*)handle;

        if ((handleData == NULL) ||
            (relativePath == NULL) ||
            (httpHeadersHandle == NULL))
        {
            result = HTTPAPI_INVALID_ARG;
            LogError("NULL parameter detected (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
        }
        else
        {
            wchar_t* requestTypeString = NULL;

            switch (requestType)
            {
            default:
                break;

            case HTTPAPI_REQUEST_GET:
                requestTypeString = L"GET";
                break;

            case HTTPAPI_REQUEST_HEAD:
                requestTypeString = L"HEAD";
                break;

            case HTTPAPI_REQUEST_POST:
                requestTypeString = L"POST";
                break;

            case HTTPAPI_REQUEST_PUT:
                requestTypeString = L"PUT";
                break;

            case HTTPAPI_REQUEST_DELETE:
                requestTypeString = L"DELETE";
                break;

            case HTTPAPI_REQUEST_PATCH:
                requestTypeString = L"PATCH";
                break;
            }

            if (requestTypeString == NULL)
            {
                result = HTTPAPI_INVALID_ARG;
                LogError("requestTypeString was NULL (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            }
            else
            {
                const char* headers2;
                headers2 = ConstructHeadersString(httpHeadersHandle);
                if (headers2 != NULL)
                {
                    size_t requiredCharactersForRelativePath = MultiByteToWideChar(CP_ACP, 0, relativePath, -1, NULL, 0);
                    wchar_t* relativePathTemp = (wchar_t*)malloc((requiredCharactersForRelativePath+1) * sizeof(wchar_t));
                    result = HTTPAPI_OK; /*legacy code*/

                    if (relativePathTemp == NULL)
                    {
                        result = HTTPAPI_ALLOC_FAILED;
                        LogError("malloc failed (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                    }
                    else
                    {
                        if (MultiByteToWideChar(CP_ACP, 0, relativePath, -1, relativePathTemp, (int)requiredCharactersForRelativePath) == 0)
                        {
                            result = HTTPAPI_STRING_PROCESSING_ERROR;
                            LogError("MultiByteToWideChar was 0. (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                        }
                        else
                        {
                            size_t requiredCharactersForHeaders = MultiByteToWideChar(CP_ACP, 0, headers2, -1, NULL, 0);

                            wchar_t* headersTemp = (wchar_t*)malloc((requiredCharactersForHeaders +1) * sizeof(wchar_t) );
                            if (headersTemp == NULL)
                            {
                                result = HTTPAPI_STRING_PROCESSING_ERROR;
                                LogError("MultiByteToWideChar was 0. (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                            }
                            else
                            {
                                if (MultiByteToWideChar(CP_ACP, 0, headers2, -1, headersTemp, (int)requiredCharactersForHeaders) == 0)
                                {
                                    result = HTTPAPI_STRING_PROCESSING_ERROR;
                                    LogError("MultiByteToWideChar was 0(result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                }
                                else
                                {
                                    HINTERNET requestHandle = WinHttpOpenRequest(
                                        handleData->ConnectionHandle,
                                        requestTypeString,
                                        relativePathTemp,
                                        NULL,
                                        WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES,
                                        WINHTTP_FLAG_SECURE);
                                    if (requestHandle == NULL)
                                    {
                                        result = HTTPAPI_OPEN_REQUEST_FAILED;
                                        LogErrorWinHTTPWithGetLastErrorAsString("WinHttpOpenRequest failed (result = %s).", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                    }
                                    else
                                    {
                                        if ((handleData->x509SchannelHandle!=NULL) &&
                                            !WinHttpSetOption(
                                                requestHandle,
                                                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                                                (void*)x509_schannel_get_certificate_context(handleData->x509SchannelHandle),
                                                sizeof(CERT_CONTEXT)
                                        ))
                                        {
                                            LogErrorWinHTTPWithGetLastErrorAsString("unable to WinHttpSetOption");
                                            result = HTTPAPI_SET_X509_FAILURE;
                                        }
                                        else
                                        {
                                            // Set proxy host if needed
                                            if (handleData->proxy_host != NULL)
                                            {
                                                WINHTTP_PROXY_INFO winhttp_proxy;
                                                wchar_t wproxy[MAX_HOSTNAME_LEN];
                                                winhttp_proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                                                if (mbstowcs_s(NULL, wproxy, MAX_HOSTNAME_LEN, handleData->proxy_host, MAX_HOSTNAME_LEN-1) != 0)
                                                {
                                                    LogError("Error during proxy host conversion");
                                                    result = HTTPAPI_ERROR;
                                                }
                                                else
                                                {
                                                    winhttp_proxy.lpszProxy = wproxy;
                                                    winhttp_proxy.lpszProxyBypass = NULL;
                                                    if (WinHttpSetOption(requestHandle,
                                                        WINHTTP_OPTION_PROXY,
                                                        &winhttp_proxy,
                                                        (DWORD)sizeof(WINHTTP_PROXY_INFO)) != TRUE)
                                                    {
                                                        LogError("failure setting proxy address (%i)", GetLastError());
                                                        result = HTTPAPI_ERROR;
                                                    }
                                                    else
                                                    {
                                                        //Set username and password if needed
                                                        if (handleData->proxy_username != NULL && handleData->proxy_password != NULL)
                                                        {
                                                            wchar_t wusername[MAX_USERNAME_LEN];
                                                            if (mbstowcs_s(NULL, wusername, MAX_USERNAME_LEN, handleData->proxy_username, MAX_USERNAME_LEN-1) != 0)
                                                            {
                                                                LogError("Error during proxy username conversion");
                                                                result = HTTPAPI_ERROR;
                                                            }
                                                            else
                                                            {
                                                                wchar_t wpassword[MAX_PASSWORD_LEN];
                                                                if (mbstowcs_s(NULL, wpassword, MAX_PASSWORD_LEN, handleData->proxy_password, MAX_PASSWORD_LEN-1) != 0)
                                                                {
                                                                    LogError("Error during proxy password conversion");
                                                                    result = HTTPAPI_ERROR;
                                                                }
                                                                else
                                                                {
                                                                    if (WinHttpSetCredentials(requestHandle,
                                                                        WINHTTP_AUTH_TARGET_PROXY,
                                                                        WINHTTP_AUTH_SCHEME_BASIC,
                                                                        wusername,
                                                                        wpassword,
                                                                        NULL) != TRUE)
                                                                    {
                                                                        LogErrorWinHTTPWithGetLastErrorAsString("Failure setting proxy credentials");
                                                                        result = HTTPAPI_ERROR;
                                                                    }
                                                                    else
                                                                    {
                                                                        result = HTTPAPI_OK;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            result = HTTPAPI_OK;
                                                        }
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                result = HTTPAPI_OK;
                                            }
                                            // verify if no error when set proxy
                                            if (result == HTTPAPI_OK)
                                            {
                                                if (WinHttpSetTimeouts(requestHandle,
                                                    0,                      /*_In_  int dwResolveTimeout - The initial value is zero, meaning no time-out (infinite). */
                                                    60000,                  /*_In_  int dwConnectTimeout, -  The initial value is 60,000 (60 seconds).*/
                                                    handleData->timeout,    /*_In_  int dwSendTimeout, -  The initial value is 30,000 (30 seconds).*/
                                                    handleData->timeout     /* int dwReceiveTimeout The initial value is 30,000 (30 seconds).*/
                                                ) == FALSE)
                                                {
                                                    result = HTTPAPI_SET_TIMEOUTS_FAILED;
                                                    LogErrorWinHTTPWithGetLastErrorAsString("WinHttpOpenRequest failed (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                }
                                                else
                                                {
                                                    DWORD dwSecurityFlags = 0;

                                                    if (!WinHttpSetOption(
                                                        requestHandle,
                                                        WINHTTP_OPTION_SECURITY_FLAGS,
                                                        &dwSecurityFlags,
                                                        sizeof(dwSecurityFlags)))
                                                    {
                                                        result = HTTPAPI_SET_OPTION_FAILED;
                                                        LogErrorWinHTTPWithGetLastErrorAsString("WinHttpSetOption failed (result = %s).", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                    }
                                                    else
                                                    {
                                                        if (!WinHttpSendRequest(
                                                            requestHandle,
                                                            headersTemp,
                                                            (DWORD)-1L, /*An unsigned long integer value that contains the length, in characters, of the additional headers. If this parameter is -1L ... */
                                                            (void*)content,
                                                            (DWORD)contentLength,
                                                            (DWORD)contentLength,
                                                            0))
                                                        {
                                                            result = HTTPAPI_SEND_REQUEST_FAILED;
                                                            LogErrorWinHTTPWithGetLastErrorAsString("WinHttpSendRequest: (result = %s).", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                        }
                                                        else
                                                        {
                                                            if (!WinHttpReceiveResponse(
                                                                requestHandle,
                                                                0))
                                                            {
                                                                result = HTTPAPI_RECEIVE_RESPONSE_FAILED;
                                                                LogErrorWinHTTPWithGetLastErrorAsString("WinHttpReceiveResponse: (result = %s).", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                            }
                                                            else
                                                            {
                                                                DWORD dwStatusCode = 0;
                                                                DWORD dwBufferLength = sizeof(DWORD);
                                                                DWORD responseBytesAvailable;

                                                                if (!WinHttpQueryHeaders(
                                                                    requestHandle,
                                                                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                                                    WINHTTP_HEADER_NAME_BY_INDEX,
                                                                    &dwStatusCode,
                                                                    &dwBufferLength,
                                                                    WINHTTP_NO_HEADER_INDEX))
                                                                {
                                                                    result = HTTPAPI_QUERY_HEADERS_FAILED;
                                                                    LogErrorWinHTTPWithGetLastErrorAsString("WinHttpQueryHeaders failed (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                }
                                                                else
                                                                {
                                                                    BUFFER_HANDLE useToReadAllResponse = (responseContent != NULL) ? responseContent : BUFFER_new();

                                                                    if (statusCode != NULL)
                                                                    {
                                                                        *statusCode = dwStatusCode;
                                                                    }

                                                                    if (useToReadAllResponse == NULL)
                                                                    {
                                                                        result = HTTPAPI_ERROR;
                                                                        LogError("(result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                    }
                                                                    else
                                                                    {

                                                                        int goOnAndReadEverything = 1;
                                                                        do
                                                                        {
                                                                            /*from MSDN: If no data is available and the end of the file has not been reached, one of two things happens. If the session is synchronous, the request waits until data becomes available.*/
                                                                            if (!WinHttpQueryDataAvailable(requestHandle, &responseBytesAvailable))
                                                                            {
                                                                                result = HTTPAPI_QUERY_DATA_AVAILABLE_FAILED;
                                                                                LogErrorWinHTTPWithGetLastErrorAsString("WinHttpQueryDataAvailable failed (result = %s).", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                goOnAndReadEverything = 0;
                                                                            }
                                                                            else if (responseBytesAvailable == 0)
                                                                            {
                                                                                /*end of the stream, go out*/
                                                                                result = HTTPAPI_OK;
                                                                                goOnAndReadEverything = 0;
                                                                            }
                                                                            else
                                                                            {
                                                                                if (BUFFER_enlarge(useToReadAllResponse, responseBytesAvailable) != 0)
                                                                                {
                                                                                    result = HTTPAPI_ERROR;
                                                                                    LogError("(result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                    goOnAndReadEverything = 0;
                                                                                }
                                                                                else
                                                                                {
                                                                                    /*Add the read bytes to the response buffer*/
                                                                                    size_t bufferSize;
                                                                                    const unsigned char* bufferContent;

                                                                                    if (BUFFER_content(useToReadAllResponse, &bufferContent) != 0)
                                                                                    {
                                                                                        result = HTTPAPI_ERROR;
                                                                                        LogError("(result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                        goOnAndReadEverything = 0;
                                                                                    }
                                                                                    else if (BUFFER_size(useToReadAllResponse, &bufferSize) != 0)
                                                                                    {
                                                                                        result = HTTPAPI_ERROR;
                                                                                        LogError("(result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                        goOnAndReadEverything = 0;
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        DWORD bytesReceived;
                                                                                        if (!WinHttpReadData(requestHandle, (LPVOID)(bufferContent + bufferSize - responseBytesAvailable), responseBytesAvailable, &bytesReceived))
                                                                                        {
                                                                                            result = HTTPAPI_READ_DATA_FAILED;
                                                                                            LogErrorWinHTTPWithGetLastErrorAsString("WinHttpReadData failed (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                            goOnAndReadEverything = 0;
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            /*if for some reason bytesReceived is zero If you are using WinHttpReadData synchronously, and the return value is TRUE and the number of bytes read is zero, the transfer has been completed and there are no more bytes to read on the handle.*/
                                                                                            if (bytesReceived == 0)
                                                                                            {
                                                                                                /*end of everything, but this looks like an error still, or a non-conformance between WinHttpQueryDataAvailable and WinHttpReadData*/
                                                                                                result = HTTPAPI_READ_DATA_FAILED;
                                                                                                LogError("bytesReceived was unexpectedly zero (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                                                goOnAndReadEverything = 0;
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                /*all is fine, keep going*/
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }

                                                                        } while (goOnAndReadEverything != 0);
                                                                    }
                                                                }

                                                                if (result == HTTPAPI_OK && responseHeadersHandle != NULL)
                                                                {
                                                                    wchar_t* responseHeadersTemp;
                                                                    DWORD responseHeadersTempLength = sizeof(responseHeadersTemp);

                                                                    (void)WinHttpQueryHeaders(
                                                                        requestHandle,
                                                                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                                        WINHTTP_HEADER_NAME_BY_INDEX,
                                                                        WINHTTP_NO_OUTPUT_BUFFER,
                                                                        &responseHeadersTempLength,
                                                                        WINHTTP_NO_HEADER_INDEX);

                                                                    responseHeadersTemp = (wchar_t*)malloc(responseHeadersTempLength + 2);
                                                                    if (responseHeadersTemp == NULL)
                                                                    {
                                                                        result = HTTPAPI_ALLOC_FAILED;
                                                                        LogError("malloc failed: (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
                                                                    }
                                                                    else
                                                                    {
                                                                        if (WinHttpQueryHeaders(
                                                                            requestHandle,
                                                                            WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                                            WINHTTP_HEADER_NAME_BY_INDEX,
                                                                            responseHeadersTemp,
                                                                            &responseHeadersTempLength,
                                                                            WINHTTP_NO_HEADER_INDEX))
                                                                        {
                                                                            wchar_t *next_token;
                                                                            wchar_t* token = wcstok_s(responseHeadersTemp, L"\r\n", &next_token);
                                                                            while ((token != NULL) &&
                                                                                (token[0] != L'\0'))
                                                                            {
                                                                                char* tokenTemp;
                                                                                size_t tokenTemp_size;

                                                                                tokenTemp_size = WideCharToMultiByte(CP_ACP, 0, token, -1, NULL, 0, NULL, NULL);
                                                                                if (tokenTemp_size == 0)
                                                                                {
                                                                                    LogError("WideCharToMultiByte failed");
                                                                                }
                                                                                else
                                                                                {
                                                                                    tokenTemp = (char*)malloc(sizeof(char)*tokenTemp_size);
                                                                                    if (tokenTemp == NULL)
                                                                                    {
                                                                                        LogError("malloc failed");
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        if (WideCharToMultiByte(CP_ACP, 0, token, -1, tokenTemp, (int)tokenTemp_size, NULL, NULL) > 0)
                                                                                        {
                                                                                            /*breaking the token in 2 parts: everything before the first ":" and everything after the first ":"*/
                                                                                            /* if there is no such character, then skip it*/
                                                                                            /*if there is a : then replace is by a '\0' and so it breaks the original string in name and value*/

                                                                                            char* whereIsColon = strchr(tokenTemp, ':');
                                                                                            if (whereIsColon != NULL)
                                                                                            {
                                                                                                *whereIsColon = '\0';
                                                                                                if (HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle, tokenTemp, whereIsColon + 1) != HTTP_HEADERS_OK)
                                                                                                {
                                                                                                    LogError("HTTPHeaders_AddHeaderNameValuePair failed");
                                                                                                    result = HTTPAPI_HTTP_HEADERS_FAILED;
                                                                                                    break;
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                            LogError("WideCharToMultiByte failed");
                                                                                        }
                                                                                        free(tokenTemp);
                                                                                    }
                                                                                }


                                                                                token = wcstok_s(NULL, L"\r\n", &next_token);
                                                                            }
                                                                        }
                                                                        else
                                                                        {
                                                                            LogError("WinHttpQueryHeaders failed");
                                                                        }

                                                                        free(responseHeadersTemp);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        (void)WinHttpCloseHandle(requestHandle);
                                    }
                                }
                                free(headersTemp);
                            }
                        }
                        free(relativePathTemp);
                    }
                    free((void*)headers2);
                }
                else
                {
                    result = HTTPAPI_ALLOC_FAILED; /*likely*/
                    LogError("ConstructHeadersString failed");
                }
            }
        }
    }

    return result;
}

HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName, const void* value)
{
    HTTPAPI_RESULT result;
    if (
        (handle == NULL) ||
        (optionName == NULL) ||
        (value == NULL)
        )
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("invalid parameter (NULL) passed to HTTPAPI_SetOption");
    }
    else
    {
        HTTP_HANDLE_DATA* httpHandleData = (HTTP_HANDLE_DATA*)handle;
        if (strcmp(OPTION_HTTP_TIMEOUT, optionName) == 0)
        {
            long timeout = (long)(*(unsigned int*)value);
            httpHandleData->timeout = timeout;
            result = HTTPAPI_OK;
        }
        else if (strcmp(SU_OPTION_X509_CERT, optionName) == 0 || strcmp(OPTION_X509_ECC_CERT, optionName) == 0)
        {
            httpHandleData->x509certificate = (const char*)value;
            if (httpHandleData->x509privatekey != NULL)
            {
                httpHandleData->x509SchannelHandle = x509_schannel_create(httpHandleData->x509certificate, httpHandleData->x509privatekey);
                if (httpHandleData->x509SchannelHandle == NULL)
                {
                    LogError("unable to x509_schannel_create");
                    result = HTTPAPI_ERROR;
                }
                else
                {
                    result = HTTPAPI_OK;
                }
            }
            else
            {
                /*if certificate comes 1st and private key is not set yet, then return OK and wait for the private key to be set*/
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0 || strcmp(OPTION_X509_ECC_KEY, optionName) == 0)
        {
            httpHandleData->x509privatekey = (const char*)value;
            if (httpHandleData->x509certificate != NULL)
            {
                httpHandleData->x509SchannelHandle = x509_schannel_create(httpHandleData->x509certificate, httpHandleData->x509privatekey);
                if (httpHandleData->x509SchannelHandle == NULL)
                {
                    LogError("unable to x509_schannel_create");
                    result = HTTPAPI_ERROR;
                }
                else
                {
                    result = HTTPAPI_OK;
                }
            }
            else
            {
                /*if privatekey comes 1st and certificate is not set yet, then return OK and wait for the certificate to be set*/
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp("TrustedCerts", optionName) == 0)
        {
            /*winhttp accepts all certificates, because it actually relies on the system ones*/
            result = HTTPAPI_OK;
        }
        else if (strcmp(OPTION_HTTP_PROXY, optionName) == 0)
        {
            char proxyAddressAndPort[MAX_HOSTNAME_LEN];
            HTTP_PROXY_OPTIONS* proxy_data = (HTTP_PROXY_OPTIONS*)value;
            if (proxy_data->host_address == NULL || proxy_data->port <= 0)
            {
              LogError("invalid proxy_data values ( host_address = %p, port = %d)", proxy_data->host_address, proxy_data->port);
              result = HTTPAPI_ERROR;
            }
            else
            {
                if (sprintf_s(proxyAddressAndPort, MAX_HOSTNAME_LEN, "%s:%d", proxy_data->host_address, proxy_data->port) <= 0)
                {
                    LogError("failure constructing proxy address");
                    result = HTTPAPI_ERROR;
                }
                else
                {
                    if(httpHandleData->proxy_host != NULL)
                    {
                      free((void*)httpHandleData->proxy_host);
                      httpHandleData->proxy_host = NULL;
                    }
                    if(mallocAndStrcpy_s((char**)&(httpHandleData->proxy_host), (const char*)proxyAddressAndPort) != 0)
                    {
                        LogError("failure allocate proxy host");
                        result = HTTPAPI_ERROR;
                    }
                    else
                    {
                        if (proxy_data->username != NULL && proxy_data->password != NULL)
                        {
                            if(httpHandleData->proxy_username != NULL)
                            {
                              free((void*)httpHandleData->proxy_username);
                              httpHandleData->proxy_username = NULL;
                            }
                            if(mallocAndStrcpy_s((char**)&(httpHandleData->proxy_username), (const char*)proxy_data->username) != 0)
                            {
                                LogError("failure allocate proxy username");
                                free((void*)httpHandleData->proxy_host);
                                httpHandleData->proxy_host = NULL;
                                result = HTTPAPI_ERROR;
                            }
                            else
                            {
                                if(httpHandleData->proxy_password != NULL)
                                {
                                  free((void*)httpHandleData->proxy_password);
                                  httpHandleData->proxy_password = NULL;
                                }
                                if(mallocAndStrcpy_s((char**)&(httpHandleData->proxy_password), (const char*)proxy_data->password) != 0)
                                {
                                    LogError("failure allocate proxy password");
                                    free((void*)httpHandleData->proxy_host);
                                    httpHandleData->proxy_host = NULL;
                                    free((void*)httpHandleData->proxy_username);
                                    httpHandleData->proxy_username = NULL;
                                    result = HTTPAPI_ERROR;
                                }
                                else
                                {
                                  result = HTTPAPI_OK;
                                }
                            }
                        }
                        else
                        {
                            result = HTTPAPI_OK;
                        }
                    }
                }
            }
        }
        else
        {
            result = HTTPAPI_INVALID_ARG;
            LogError("unknown option %s", optionName);
        }
    }
    return result;
}

HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value, const void** savedValue)
{
    HTTPAPI_RESULT result;
    if (
        (optionName == NULL) ||
        (value == NULL) ||
        (savedValue == NULL)
        )
    {
        result = HTTPAPI_INVALID_ARG;
        LogError("invalid argument(NULL) passed to HTTPAPI_CloneOption");
    }
    else
    {
        if (strcmp(OPTION_HTTP_TIMEOUT, optionName) == 0)
        {
            /*by convention value is pointing to an unsigned int */
            unsigned int* temp = (unsigned int*)malloc(sizeof(unsigned int)); /*shall be freed by HTTPAPIEX*/
            if (temp == NULL)
            {
                result = HTTPAPI_ERROR;
                LogError("malloc failed (result = %s)", ENUM_TO_STRING(HTTPAPI_RESULT, result));
            }
            else
            {
                *temp = *(const unsigned int*)value;
                *savedValue = temp;
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp(SU_OPTION_X509_CERT, optionName) == 0 || strcmp(OPTION_X509_ECC_CERT, optionName) == 0)
        {
            /*this is getting the x509 certificate. In this case, value is a pointer to a const char* that contains the certificate as a null terminated string*/
            if (mallocAndStrcpy_s((char**)savedValue, (const char*)value) != 0)
            {
                LogError("unable to clone the x509 certificate content");
                result = HTTPAPI_ERROR;
            }
            else
            {
                /*return OK when the certificate has been clones successfully*/
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0 || strcmp(OPTION_X509_ECC_KEY, optionName) == 0)
        {
            /*this is getting the x509 private key. In this case, value is a pointer to a const char* that contains the private key as a null terminated string*/
            if (mallocAndStrcpy_s((char**)savedValue, (const char*)value) != 0)
            {
                LogError("unable to clone the x509 private key content");
                result = HTTPAPI_ERROR;
            }
            else
            {
                /*return OK when the private key has been clones successfully*/
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp("TrustedCerts", optionName) == 0)
        {
            *savedValue = malloc(1); /*_CloneOption needs to return in *savedValue something that can be free()'d*/
            if (*savedValue == NULL)
            {
                LogError("failure in malloc");
                result = HTTPAPI_ERROR;
            }
            else
            {
                result = HTTPAPI_OK;
            }
        }
        else if (strcmp(OPTION_HTTP_PROXY, optionName) == 0)
        {
            *savedValue = malloc(sizeof(HTTP_PROXY_OPTIONS));
            if (*savedValue == NULL)
            {
                LogError("failure in malloc");
                result = HTTPAPI_ERROR;
            }
            else
            {
                HTTP_PROXY_OPTIONS *savedOption = (HTTP_PROXY_OPTIONS*) *savedValue;
                HTTP_PROXY_OPTIONS *option = (HTTP_PROXY_OPTIONS*) value;
                memset((void*)*savedValue, 0, sizeof(HTTP_PROXY_OPTIONS));
                if (mallocAndStrcpy_s((char**)&(savedOption->host_address),
                                      (const char*)option->host_address) != 0)
                {
                  LogError("unable to clone the proxy host adress content");
                  free((void*)*savedValue);
                  *savedValue = NULL;
                  result = HTTPAPI_ERROR;
                }
                else
                {
                    savedOption->port = option->port;
                    if (option->username != NULL && mallocAndStrcpy_s((char**)&(savedOption->username),
                                                                      (const char*)option->username) != 0)
                    {
                      LogError("unable to clone the proxy username content");
                      free((void*)savedOption->host_address);
                      savedOption->host_address = NULL;
                      free((void*)*savedValue);
                      *savedValue = NULL;
                      result = HTTPAPI_ERROR;
                    }
                    else
                    {
                        if (option->password != NULL && mallocAndStrcpy_s((char**)&(savedOption->password),
                                                                          (const char*)option->password) != 0)
                        {
                          LogError("unable to clone the proxy password content");
                          free((void*)savedOption->host_address);
                          savedOption->host_address = NULL;
                          free((void*)savedOption->username);
                          savedOption->username = NULL;
                          free((void*)*savedValue);
                          *savedValue = NULL;
                          result = HTTPAPI_ERROR;
                        }
                        else
                        {
                            result = HTTPAPI_OK;
                        }
                    }
                }
            }
        }
        else
        {
            result = HTTPAPI_INVALID_ARG;
            LogError("unknown option %s", optionName);
        }
    }
    return result;
}
