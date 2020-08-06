// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../common/iotc_internal.h"
#if !defined(USE_LIGHT_CLIENT)
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include "../common/json.h"

#ifdef TARGET_MXCHIP
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include <AzureIotHub.h>
#include "provisioning_client/adapters/hsm_client_key.h"
void IOTC_LOG(const __FlashStringHelper *format, ...) {
    if (getLogLevel() > IOTC_LOGGING_DISABLED) {
        va_list ap;
        va_start(ap, format);
        AzureIOT::StringBuffer buffer(STRING_BUFFER_1024);
        buffer.setLength(vsnprintf(*buffer, STRING_BUFFER_1024, (const char *)format, ap));
        Serial.println(*buffer);
        va_end(ap);
    }
}

#elif defined(ESP_PLATFORM)
#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "iothubtransportmqtt.h"
#include "iothub_client_version.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"

#else
#error "NOT IMPLEMENTED"
#endif // TARGET_MXCHIP?

typedef struct EVENT_INSTANCE_TAG {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    IOTContextInternal *internal;
    void *appContext;
} EVENT_INSTANCE;

static EVENT_INSTANCE *createEventInstance(IOTContextInternal *internal,
    const char* payload, unsigned length, void *applicationContext, int *errorCode) {
    *errorCode = 0;

    EVENT_INSTANCE *currentMessage = (EVENT_INSTANCE*) malloc(sizeof(EVENT_INSTANCE));
    if(currentMessage == NULL) {
        IOTC_LOG(F("ERROR: (createEventInstance) currentMessage is NULL."));
        *errorCode = 1;
        return NULL;
    }
    memset(currentMessage, 0, sizeof(EVENT_INSTANCE));

    currentMessage->messageHandle =
        IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, length);

    if (currentMessage->messageHandle == NULL) {
        IOTC_LOG(F("ERROR: (iotc_send_telemetry) IoTHubMessage_CreateFromByteArray has failed."));
        free(currentMessage);
        *errorCode = 9;
        return NULL;
    }

    // set the message content type to application/json and utf-8
    (void)IoTHubMessage_SetContentTypeSystemProperty(currentMessage->messageHandle, "application%2fjson");
    (void)IoTHubMessage_SetContentEncodingSystemProperty(currentMessage->messageHandle, "utf-8");

    currentMessage->internal = internal;
    currentMessage->appContext = applicationContext;

    return currentMessage;
}

static void freeEventInstance(EVENT_INSTANCE *eventInstance) {
    if (eventInstance != NULL) {
        if (eventInstance->messageHandle != NULL) {
            IoTHubMessage_Destroy(eventInstance->messageHandle);
        }
        free(eventInstance);
    }
}

// send telemetry etc. confirmation callback
/* MessageSent */
static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;
    assert(eventInstance != NULL);
    IOTContextInternal *internal = (IOTContextInternal*)eventInstance->internal;
    const unsigned char* buffer = NULL;
    size_t size = 0;

    if (internal->callbacks[/*IOTCallbacks::*/MessageSent].callback) {
        if (IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK !=
          IoTHubMessage_GetByteArray(eventInstance->messageHandle, &buffer, &size)) {
            IOTC_LOG(F("ERROR: (sendConfirmationCallback) IoTHubMessage_GetByteArray has failed."));
        }

        IOTCallbackInfo info;
        info.eventName = "MessageSent";
        info.tag = NULL;
        info.payload = (const char*) buffer;
        info.payloadLength = (unsigned) size;
        info.appContext = internal->callbacks[/*IOTCallbacks::*/::MessageSent].appContext;
        info.statusCode = (int)result;
        info.callbackResponse = NULL;
        internal->callbacks[/*IOTCallbacks::*/::MessageSent].callback(internal, &info);
    }

    freeEventInstance(eventInstance);
}

#define CONVERT_TO_IOTHUB_MESSAGE(x) \
  (x == IOTC_MESSAGE_ACCEPTED ? IOTHUBMESSAGE_ACCEPTED : \
  (x == IOTC_MESSAGE_REJECTED ? IOTHUBMESSAGE_REJECTED : IOTHUBMESSAGE_ABANDONED))

/* Command */
static const char* emptyResponse = "{}";
static int onCommand(const char* method_name, const unsigned char* payload,
    size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback) {

    assert(response != NULL && resp_size != NULL);
    *response = NULL;
    *resp_size = 0;
    IOTContextInternal *internal = (IOTContextInternal*)userContextCallback;
    assert(internal != NULL);

    if (internal->callbacks[/*IOTCallbacks::*/::Command].callback) {
        IOTCallbackInfo info;
        info.eventName = "Command";
        info.tag = method_name;
        info.payload = (const char*) payload;
        info.payloadLength = (unsigned) size;
        info.appContext = internal->callbacks[/*IOTCallbacks::*/::Command].appContext;
        info.statusCode = 0;
        info.callbackResponse = NULL;
        internal->callbacks[/*IOTCallbacks::*/::Command].callback(internal, &info);

        if (info.callbackResponse != NULL) {
            *response = (unsigned char*) info.callbackResponse;
            *resp_size = strlen((char*) info.callbackResponse);
        } else {
            char *resp = (char*) malloc(3);
            resp[2] = 0;
            memcpy(resp, emptyResponse, 2);
            *response = (unsigned char*)resp;
            *resp_size = 2;
            // resp should be freed by SDK
        }
        return 200;
    }

    return 500;
}

/* MessageSent */
static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback) {
    // TODO: use status code
    sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_OK, userContextCallback);
}

#define CONVERT_TO_IOTC_CONNECT_ENUM(x) \
  ( x == IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN ? IOTC_CONNECTION_EXPIRED_SAS_TOKEN : ( \
      x == IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED ? IOTC_CONNECTION_RETRY_EXPIRED : (   \
        x == IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED ? IOTC_CONNECTION_DEVICE_DISABLED : ( \
          x == IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL ? IOTC_CONNECTION_BAD_CREDENTIAL : ( \
            x == IOTHUB_CLIENT_CONNECTION_NO_NETWORK ? IOTC_CONNECTION_NO_NETWORK : ( \
              x == IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR ? IOTC_CONNECTION_COMMUNICATION_ERROR : IOTC_CONNECTION_OK \
            ))))))

/* ConnectionStatus */
static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {

    IOTContextInternal *internal = (IOTContextInternal*)userContextCallback;
    assert(internal != NULL);
    if (internal->callbacks[/*IOTCallbacks::*/::ConnectionStatus].callback) {
        IOTCallbackInfo info;
        info.eventName = "ConnectionStatus";
        info.tag = NULL;
        info.payload = NULL;
        info.payloadLength = 0;
        info.appContext = internal->callbacks[/*IOTCallbacks::*/::ConnectionStatus].appContext;
        info.statusCode = CONVERT_TO_IOTC_CONNECT_ENUM(reason);
        info.callbackResponse = NULL;
        internal->callbacks[/*IOTCallbacks::*/::ConnectionStatus].callback(internal, &info);
    }
}

void sendOnError(IOTContextInternal *internal, const char* message) {
    if (internal->callbacks[/*IOTCallbacks::*/::Error].callback) {
        IOTCallbackInfo info;
        info.eventName = "Error";
        info.tag = message; // message lifetime should be managed by us
        info.payload = NULL;
        info.payloadLength = 0;
        info.appContext = internal->callbacks[/*IOTCallbacks::*/::Error].appContext;
        info.statusCode = 1;
        info.callbackResponse = NULL;
        internal->callbacks[/*IOTCallbacks::*/::Error].callback(internal, &info);
    }
}

void echoDesired(IOTContextInternal *internal, const char *propertyName,
  const char *message, const char *status, int statusCode) {
    AzureIOT::JSObject rootObject(message);
    AzureIOT::JSObject propertyNameObject, desiredObject, desiredObjectPropertyName;

    const char* methodName = rootObject.getStringByName("methodName");
    rootObject.getObjectByName(propertyName, &propertyNameObject);

    double value = 0, desiredVersion = 0;
    if (rootObject.getObjectByName("desired", &desiredObject) &&
        desiredObject.getObjectByName(propertyName, &desiredObjectPropertyName)) {

        value = desiredObjectPropertyName.getNumberByName("value");
        desiredVersion = desiredObject.getNumberByName("$version");
    } else {
        propertyName = rootObject.getNameAt(0);

        value = propertyNameObject.getNumberByName("value");
        desiredVersion = rootObject.getNumberByName("$version");
    }

    const char* echoTemplate = "{\"%s\":{\"value\":%d,\"statusCode\":%d,\
\"status\":\"%s\",\"desiredVersion\":%d}}";
    uint32_t buffer_size = strlen(echoTemplate) + 23 /* x64 value */ + 3 /* statusCode */
        + 32 /* status */ + 23 /* version max */;
    buffer_size = iotc_min(buffer_size, 512);
    AzureIOT::StringBuffer buffer(buffer_size);

    size_t size = snprintf(*buffer, buffer_size, echoTemplate, propertyName,
             (int) value, statusCode, status, (int) desiredVersion);
    buffer.setLength(size);

    iotc_send_property(internal, *buffer, size);
}

void callDesiredCallback(IOTContextInternal *internal, const char *propertyName, const char *payLoad, size_t size) {
    const char* response = "completed";
    if (internal->callbacks[/*IOTCallbacks::*/::SettingsUpdated].callback) {
        IOTCallbackInfo info;
        info.eventName = "SettingsUpdated";
        info.tag = propertyName;
        info.payload = payLoad;
        info.payloadLength = size;
        info.appContext = internal->callbacks[/*IOTCallbacks::*/::SettingsUpdated].appContext;
        info.statusCode = 200;
        info.callbackResponse = NULL;
        internal->callbacks[/*IOTCallbacks::*/::SettingsUpdated].callback(internal, &info);
        if (info.callbackResponse) {
            response = (const char*)info.callbackResponse;
        }
        echoDesired(internal, propertyName, payLoad, response, info.statusCode);
    }
}

static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state,
    const unsigned char* payLoad, size_t size, void* userContextCallback) {

    IOTContextInternal *internal = (IOTContextInternal*)userContextCallback;
    assert(internal != NULL);

    ((char*)payLoad)[size] = 0x00;
    AzureIOT::JSObject payloadObject((const char *)payLoad);

    if (update_state == DEVICE_TWIN_UPDATE_PARTIAL && payloadObject.getNameAt(0) != NULL) {
        callDesiredCallback(internal, payloadObject.getNameAt(0), reinterpret_cast<const char*>(payLoad), size);
    } else {
        AzureIOT::JSObject desired, reported;

        // loop through all the desired properties
        // look to see if the desired property has an associated reported property
        // if so look if the versions match, if they match do nothing
        // if they don't match then call the associated callback for the desired property

        payloadObject.getObjectByName("desired", &desired);
        payloadObject.getObjectByName("reported", &reported);

        for (unsigned i = 0, count = desired.getCount(); i < count; i++) {
            const char * itemName = desired.getNameAt(i);
            if (itemName != NULL && itemName[0] != '$') {
                AzureIOT::JSObject keyObject;
                const char * version = NULL, * desiredVersion = NULL,
                           * value = NULL, * desiredValue = NULL;
                bool containsKey = reported.getObjectByName(itemName, &keyObject);

                if (containsKey) {
                    desiredVersion = reported.getStringByName("desiredVersion");
                    version = desired.getStringByName("$version");
                    desiredValue = reported.getStringByName("desiredValue");
                    value = desired.getStringByName("value");
                }

                if (containsKey && strcmp(desiredVersion, version) == 0) {
                    IOTC_LOG(F("key: %s found in reported and versions match"), itemName);
                } else if (containsKey && strcmp(desiredValue, value) != 0){
                    IOTC_LOG(F("key: %s either not found in reported or versions do not match\r\n"), itemName);
                    AzureIOT::JSObject itemValue;
                    if (desired.getObjectAt(i, &itemValue) && itemValue.toString() != NULL) {
                        IOTC_LOG(F("itemValue: %s"), itemValue.toString());
                    } else {
                        IOTC_LOG(F("ERROR: desired doesn't have value at index"));
                    }
                    callDesiredCallback(internal, itemName, (const char*)payLoad, size);
                } else {
                    echoDesired(internal, itemName, (const char*)payLoad, "completed", 200);
                }
            }
        }
    }
}

/* extern */
int iotc_init_context(IOTContext *ctx) {
    CHECK_NOT_NULL(ctx)

    MUST_CALL_BEFORE_INIT((*ctx));
    IOTContextInternal *internal = (IOTContextInternal*)malloc(sizeof(IOTContextInternal));
    CHECK_NOT_NULL(internal);
    memset(internal, 0, sizeof(IOTContextInternal));
    *ctx = (void*)internal;

    return 0;
}

/* extern */
int iotc_free_context(IOTContext ctx) {
    MUST_CALL_AFTER_INIT(ctx);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    if (internal->endpoint != NULL) {
        free(internal->endpoint);
    }

    if (internal->clientHandle != NULL) {
        IoTHubClient_LL_Destroy(internal->clientHandle);
    }

    free(internal);

    return 0;
}

typedef struct REGISTRATION_CONTEXT_TAG
{
    char* iothub_uri;
    int registration_complete;
} REGISTRATION_CONTEXT;

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        IOTC_LOG(F("ERROR: (registation_status_callback) user_context is NULL"));
    }
    else
    {
        if (reg_status == PROV_DEVICE_REG_STATUS_CONNECTED)
        {
            IOTC_LOG(F("IOTC: Registration status: CONNECTED"));
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_REGISTERING)
        {
            IOTC_LOG(F("IOTC: Registration status: REGISTERING"));
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_ASSIGNING)
        {
            IOTC_LOG(F("IOTC: Registration status: ASSIGNING"));
        }
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* deviceId, void* user_context)
{
    if (user_context == NULL)
    {
        IOTC_LOG(F("ERROR: (register_device_callback) user_context is NULL"));
    }
    else
    {
        REGISTRATION_CONTEXT* user_ctx = (REGISTRATION_CONTEXT*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            user_ctx->iothub_uri = strdup(iothub_uri);
            user_ctx->registration_complete = 1;
        }
        else
        {
            IOTC_LOG(F("ERROR: (register_device_callback) Failure encountered on registration!"));
            user_ctx->registration_complete = 2;
        }
    }
}

/* extern */
int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
  const char* deviceId, IOTConnectType type) {

    CHECK_NOT_NULL(ctx)
    GET_LENGTH_NOT_NULL(scope, 256);
    GET_LENGTH_NOT_NULL(keyORcert, 512);
    GET_LENGTH_NOT_NULL(deviceId, 256);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

    char stringBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    int errorCode = 0;
    size_t pos = 0;
    bool traceOn = getLogLevel() > IOTC_LOGGING_API_ONLY;

    if (type == IOTC_CONNECT_CONNECTION_STRING) {
        strcpy(stringBuffer, keyORcert);
        pos = strlen(stringBuffer);
    } else {
        if (type == IOTC_CONNECT_SYMM_KEY) {
            prov_dev_set_symmetric_key_info(deviceId, keyORcert);
            prov_dev_security_init(SECURE_DEVICE_TYPE_SYMMETRIC_KEY);
        } else {
            prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
        }
        PROV_DEVICE_LL_HANDLE handle;
        if ((handle = Prov_Device_LL_Create(internal->endpoint == NULL ?
            DEFAULT_ENDPOINT : internal->endpoint, scope, Prov_Device_MQTT_Protocol)) == NULL)
        {
            IOTC_LOG(F("ERROR: (iotc_connect) device registration step has failed."));
            errorCode = 1;
            goto fnc_exit;
        }

        REGISTRATION_CONTEXT user_ctx = { 0 };

        Prov_Device_LL_SetOption(handle, "logtrace", &traceOn);
#if defined(MBED_BUILD_TIMESTAMP) || defined(TARGET_MXCHIP_AZ3166)
        if (Prov_Device_LL_SetOption(handle, "TrustedCerts", certificates) != PROV_DEVICE_RESULT_OK)
        {
            IOTC_LOG(F("ERROR: (iotc_connect) Failed to set option \"TrustedCerts\"."));
            errorCode = 1;
            goto fnc_exit;
        } else
#endif // defined(MBED_BUILD_TIMESTAMP) || defined(TARGET_MXCHIP_AZ3166)
        if (Prov_Device_LL_Register_Device(handle, register_device_callback, &user_ctx, registation_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
        {
            IOTC_LOG(F("ERROR: (iotc_connect) Failed calling Prov_Device_LL_Register_Device."));
            errorCode = 1;
            goto fnc_exit;
        }
        else
        {
            // Waiting the register to be completed
            do
            {
                Prov_Device_LL_DoWork(handle);
                ThreadAPI_Sleep(5);
            } while (user_ctx.registration_complete == 0);
        }
        // Free DPS client
        Prov_Device_LL_Destroy(handle);

        if (user_ctx.registration_complete == 2) {
            IOTC_LOG(F("ERROR: (iotc_connect) device registration step has failed."));
            errorCode = 1;
            goto fnc_exit;
        }

        if (type == IOTC_CONNECT_SYMM_KEY) {
            pos = snprintf(stringBuffer, AZ_IOT_HUB_MAX_LEN,
                "HostName=%s;DeviceId=%s;SharedAccessKey=%s",
                user_ctx.iothub_uri,
                deviceId,
                keyORcert);
        } else if (type == IOTC_CONNECT_X509_CERT) {
            pos = snprintf(stringBuffer, AZ_IOT_HUB_MAX_LEN,
                "HostName=%s;DeviceId=%s;UseProvisioning=true",
                user_ctx.iothub_uri,
                deviceId);
        }
    }

    IOTC_LOG(F("ConnectionString: %s"), stringBuffer);
    if (pos == 0 || pos >= AZ_IOT_HUB_MAX_LEN) {
        IOTC_LOG(F("ERROR: (iotc_connect) connection information is out of buffer."));
        errorCode = 15;
        goto fnc_exit;
    }

    if ((internal->clientHandle = IoTHubClient_LL_CreateFromConnectionString(
        stringBuffer, MQTT_Protocol)) == NULL) {

        IOTC_LOG(F("ERROR: (iotc_connect) Couldn't create hub from connection string."));
        errorCode = 4;
        goto fnc_exit;
    }

    IoTHubClient_LL_SetRetryPolicy(internal->clientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);

#if defined(MBED_BUILD_TIMESTAMP) || defined(TARGET_MXCHIP_AZ3166)
    if (IoTHubClient_LL_SetOption(internal->clientHandle, "TrustedCerts",
        certificates /* src/cores/arduino/az_iot/azureiotcerts.h */) != IOTHUB_CLIENT_OK) {
        IOTC_LOG(F("ERROR: Failed to set option \"TrustedCerts\" IoTHubClient_LL_SetOption failed."));
        return 5;
    }
#endif // defined(MBED_BUILD_TIMESTAMP) || defined(TARGET_MXCHIP_AZ3166)

    traceOn = getLogLevel() > IOTC_LOGGING_API_ONLY;
    IoTHubClient_LL_SetOption(internal->clientHandle, "logtrace", &traceOn);

    if (IoTHubClient_LL_SetDeviceTwinCallback(internal->clientHandle, deviceTwinGetStateCallback,
        internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG(F("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed."));
        errorCode = 5;
        goto fnc_exit;
    }

    if (IoTHubClient_LL_SetDeviceMethodCallback(internal->clientHandle, onCommand,
        internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG(F("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005"));
        errorCode = 5;
        goto fnc_exit;
    }

    if (IoTHubClient_LL_SetConnectionStatusCallback(internal->clientHandle,
        connectionStatusCallback, internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG(F("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005"));
        errorCode = 5;
        goto fnc_exit;
    }

fnc_exit:
    return errorCode;
}

/* extern */
int iotc_disconnect(IOTContext ctx) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    IOTC_LOG(F("ERROR: (iotc_disconnect) Not implemented."));
    return 1;
}

/* extern */
int iotc_send_telemetry(IOTContext ctx, const char* payload, unsigned length) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    int errorCode = 0;
    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;
    EVENT_INSTANCE *currentMessage = createEventInstance(internal, payload, length, NULL, &errorCode);
    if (currentMessage == NULL) return errorCode;

    MAP_HANDLE propMap = IoTHubMessage_Properties(currentMessage->messageHandle);

    time_t now_utc = time(NULL); // utc time
    char timeBuffer[128] = {0};
    unsigned outputLength = snprintf(timeBuffer, 128, "%s", ctime(&now_utc));
    assert(outputLength && outputLength < 128 && timeBuffer[outputLength - 1] == '\n');
    timeBuffer[outputLength - 1] = char(0); // replace `\n` with `\0`
    if (Map_AddOrUpdate(propMap, "timestamp", timeBuffer) != MAP_OK)
    {
        IOTC_LOG(F("ERROR: (iotc_send_telemetry) Map_AddOrUpdate has failed."));
        freeEventInstance(currentMessage);
        return 1;
    }

    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(internal->clientHandle,
        currentMessage->messageHandle, sendConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        IOTC_LOG(F("ERROR: (iotc_send_telemetry) IoTHubClient_LL_SendEventAsync has "
          "failed => hubResult is (%d)."), hubResult);
        freeEventInstance(currentMessage);
        return 1;
    }

    iotc_do_work(ctx);

    return 0;
}

/* extern */
int iotc_send_property (IOTContext ctx, const char* payload, unsigned length) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    int errorCode = 0;
    EVENT_INSTANCE *currentMessage = createEventInstance(internal, payload, length, NULL, &errorCode);
    if (currentMessage == NULL) return errorCode;

    IOTHUB_CLIENT_RESULT hubResult = IoTHubClient_LL_SendReportedState(internal->clientHandle,
        (const unsigned char*)payload, length, deviceTwinConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        IOTC_LOG(F("ERROR: (iotc_send_telemetry) IoTHubClient_LL_SendReportedState has "
          "failed => hubResult is (%d). ERROR:0x000B"), hubResult);
        freeEventInstance(currentMessage);
        return 11;
    }
    iotc_do_work(ctx);

    return 0;
}

/* extern */
int iotc_do_work(IOTContext ctx) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    IoTHubClient_LL_DoWork(internal->clientHandle);
    return 0;
}

/* extern */
int iotc_set_network_interface(void* networkInterface) {
    IOTC_LOG(F("ERROR: (iotc_set_network_interface) is not needed for this platform."));
    return 1;
}

#endif // defined(TARGET_MXCHIP_AZ3166) || defined(ESP_PLATFORM)