// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include "../inc/iotc.h"
#include "../inc/utility.h"

// MXCHIP
#define MXCHIP_AZ3166
#ifdef MXCHIP_AZ3166
#include <DevkitDPSClient.h>
#include <AzureIotHub.h>
#endif // MXCHIP_AZ3166

#define AZ_IOT_HUB_MAX_LEN 1024
#define DEFAULT_ENDPOINT "global.azure-devices-provisioning.net"
#define TO_STR_(s) #s
#define TO_STR(s) TO_STR_(s)
#define IOTC_LOG(...) \
    if (gLogLevel > IOTC_LOGGING_DISABLED) { \
        printf("  - "); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    }

typedef enum IOTCallbacks_TAG {
    ConnectionStatus    = 0x01,
    MessageSent,
    Command,
    MessageReceived,
    Error,
    SettingsUpdated
} IOTCallbacks;

typedef struct CallbackBase_TAG {
  IOTCallback callback;
  void *appContext;
  CallbackBase_TAG() { callback = NULL; appContext = NULL; }
} CallbackBase;

typedef struct IOTContextInternal_TAG {
    IOTHUB_CLIENT_LL_HANDLE clientHandle;
    char *endpoint;
    IOTProtocol protocol;
    CallbackBase callbacks[8];
} IOTContextInternal;
IOTLogLevel gLogLevel = IOTC_LOGGING_DISABLED;

unsigned strlen_s_(const char* str, int max_expected) {
    int ret_val = 0;
    while(*(str++) != 0) {
        ret_val++;
        if (ret_val >= max_expected) return max_expected;
    }

    return ret_val;
}

#define CHECK_NOT_NULL(x) if (x == NULL) { IOTC_LOG(TO_STR(x) "is NULL"); return 1; }

#define GET_LENGTH_NOT_NULL_NOT_EMPTY(x, maxlen) \
unsigned x ## _len = 0; \
do { \
    CHECK_NOT_NULL(x) \
    x ## _len = strlen_s_(x, INT_MAX); \
    if (x ## _len == 0 || x ## _len > maxlen) { \
        IOTC_LOG(TO_STR(x) "has length %d", x ## _len); return 1; \
    } \
} while(0)

#define MUST_CALL_BEFORE_INIT(x) \
    if (x != NULL) {    \
        IOTC_LOG("ERROR: Client was already initialized. ERR:0x0006"); \
        return 6; \
    }

#define MUST_CALL_AFTER_INIT(x) \
    if (x == NULL) {    \
        IOTC_LOG("ERROR: Client was not initialized. ERR:0x0007"); \
        return 7; \
    }

#define MUST_CALL_AFTER_CONNECT(x) \
    if (x == NULL || x->clientHandle == NULL) {    \
        IOTC_LOG("ERROR: Client was not connected. ERR:0x0010"); \
        return 16; \
    }

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
        IOTC_LOG("ERROR: (createEventInstance) currentMessage is NULL. ERROR:0x0001");
        *errorCode = 1;
        return NULL;
    }
    memset(currentMessage, 0, sizeof(EVENT_INSTANCE));

    currentMessage->messageHandle =
        IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, length);

    if (currentMessage->messageHandle == NULL) {
        IOTC_LOG("ERROR: (iotc_send_telemetry) IoTHubMessage_CreateFromByteArray has failed. ERROR:0x0009");
        free(currentMessage);
        *errorCode = 9;
        return NULL;
    }
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

    if (internal->callbacks[IOTCallbacks::MessageSent].callback) {
        const unsigned char* buffer = NULL;
        size_t size = 0;
        if (IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK !=
          IoTHubMessage_GetByteArray(eventInstance->messageHandle, &buffer, &size)) {
            IOTC_LOG("ERROR: (sendConfirmationCallback) IoTHubMessage_GetByteArray has failed. ERR:0x000C");
        }

        IOTCallbackInfo info;
        info.eventName = "MessageSent";
        info.tag = NULL;
        info.payload = (const char*) buffer;
        info.payload_length = (unsigned) size;
        info.appContext = eventInstance->appContext;
        info.statusCode = (int)result;
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::MessageSent].callback(internal, info);
    }

    freeEventInstance(eventInstance);
}

#define CONVERT_TO_IOTHUB_MESSAGE(x) \
  (x == IOTC_MESSAGE_ACCEPTED ? IOTHUBMESSAGE_ACCEPTED : \
  (x == IOTC_MESSAGE_REJECTED ? IOTHUBMESSAGE_REJECTED : IOTHUBMESSAGE_ABANDONED))

/* MessageReceived */
static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback
    (IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    IOTContextInternal *internal = (IOTContextInternal*)userContextCallback;
    assert(internal != NULL);

    if (internal->callbacks[IOTCallbacks::MessageReceived].callback) {
        const unsigned char* buffer = NULL;
        size_t size = 0;
        if (IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK !=
          IoTHubMessage_GetByteArray(message, &buffer, &size)) {
            IOTC_LOG("ERROR: (receiveMessageCallback) IoTHubMessage_GetByteArray has failed. ERR:0x000C");
        }

        IOTCallbackInfo info;
        info.eventName = "MessageReceived";
        info.tag = NULL;
        info.payload = (const char*) buffer;
        info.payload_length = (unsigned) size;
        info.appContext = internal->callbacks[IOTCallbacks::MessageReceived].appContext;
        info.statusCode = 0;
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::MessageReceived].callback(internal, info);
        if (info.statusCode != 0) {
            return CONVERT_TO_IOTHUB_MESSAGE(info.statusCode);
        }
        return IOTHUBMESSAGE_ACCEPTED;
    }
    return IOTHUBMESSAGE_ABANDONED;
}

/* Command */
static const char* emptyResponse = "{}";
static int onCommand(const char* method_name, const unsigned char* payload,
    size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback) {

    assert(response != NULL && resp_size != NULL);
    *response = NULL;
    *resp_size = 0;
    IOTContextInternal *internal = (IOTContextInternal*)userContextCallback;
    assert(internal != NULL);

    if (internal->callbacks[IOTCallbacks::Command].callback) {
        IOTCallbackInfo info;
        info.eventName = "Command";
        info.tag = method_name;
        info.payload = (const char*) payload;
        info.payload_length = (unsigned) size;
        info.appContext = internal->callbacks[IOTCallbacks::Command].appContext;
        info.statusCode = 0;
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::Command].callback(internal, info);

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
    if (internal->callbacks[IOTCallbacks::ConnectionStatus].callback) {
        IOTCallbackInfo info;
        info.eventName = "ConnectionStatus";
        info.tag = NULL;
        info.payload = NULL;
        info.payload_length = 0;
        info.appContext = internal->callbacks[IOTCallbacks::ConnectionStatus].appContext;
        info.statusCode = CONVERT_TO_IOTC_CONNECT_ENUM(reason);
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::ConnectionStatus].callback(internal, info);
    }
}

static const char* OOM_MESSAGE = "OutOfMemory while handling twin message echo.";
void sendOnError(IOTContextInternal *internal, const char* message) {
    if (internal->callbacks[IOTCallbacks::Error].callback) {
        IOTCallbackInfo info;
        info.eventName = "Error";
        info.tag = message; // message lifetime should be managed by us
        info.payload = NULL;
        info.payload_length = 0;
        info.appContext = internal->callbacks[IOTCallbacks::Error].appContext;
        info.statusCode = 1;
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::Error].callback(internal, info);
    }
}

void echoDesired(IOTContextInternal *internal, const char *propertyName,
  const char *message, const char *status, int statusCode) {
    JSObject rootObject(message);
    JSObject propertyNameObject, desiredObject, desiredObjectPropertyName;

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
    uint32_t buffer_size = snprintf(NULL, 0, echoTemplate, propertyName,
        (int) value, // BAD HACK
        statusCode, status, (int) desiredVersion);

    char *buffer = new char[buffer_size + 1];
    memset(buffer, 0, buffer_size + 1);
    if (buffer == NULL) {
        IOTC_LOG("Desired property %s failed to be echoed back as a reported \
            property (OUT OF MEMORY)", propertyName);
        sendOnError(internal, OOM_MESSAGE);
        return;
    }

    snprintf(buffer, buffer_size + 1, echoTemplate, propertyName,
             (int) value, statusCode,
             status, (int) desiredVersion);

    iotc_send_property(internal, buffer, buffer_size, NULL);
}

void callDesiredCallback(IOTContextInternal *internal, const char *propertyName, const char *payLoad, size_t size) {
    const char* response = "completed";
    if (internal->callbacks[IOTCallbacks::SettingsUpdated].callback) {
        IOTCallbackInfo info;
        info.eventName = "SettingsUpdated";
        info.tag = propertyName;
        info.payload = payLoad;
        info.payload_length = size;
        info.appContext = internal->callbacks[IOTCallbacks::SettingsUpdated].appContext;
        info.statusCode = 200;
        info.callbackResponse = NULL;
        internal->callbacks[IOTCallbacks::SettingsUpdated].callback(internal, info);
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
    JSObject payloadObject((const char *)payLoad);

    if (update_state == DEVICE_TWIN_UPDATE_PARTIAL && payloadObject.getNameAt(0) != NULL) {
        callDesiredCallback(internal, payloadObject.getNameAt(0), reinterpret_cast<const char*>(payLoad), size);
    } else {
        JSObject desired, reported;

        // loop through all the desired properties
        // look to see if the desired property has an associated reported property
        // if so look if the versions match, if they match do nothing
        // if they don't match then call the associated callback for the desired property

        LOG_VERBOSE("Processing complete twin");

        payloadObject.getObjectByName("desired", &desired);
        payloadObject.getObjectByName("reported", &reported);

        for (unsigned i = 0, count = desired.getCount(); i < count; i++) {
            const char * itemName = desired.getNameAt(i);
            if (itemName != NULL && itemName[0] != '$') {
                JSObject keyObject;
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
                    LOG_VERBOSE("key: %s found in reported and versions match", itemName);
                } else if (containsKey && strcmp(desiredValue, value) != 0){
                    LOG_VERBOSE("key: %s either not found in reported or versions do not match\r\n", itemName);
                    JSObject itemValue;
                    if (desired.getObjectAt(i, &itemValue) && itemValue.toString() != NULL) {
                        LOG_VERBOSE("itemValue: %s", itemValue.toString());
                    } else {
                        LOG_ERROR("desired doesn't have value at index");
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
int iotc_set_logging(IOTLogLevel level) {
    if (level < IOTC_LOGGING_DISABLED || level > IOTC_LOGGING_ALL) {
        IOTC_LOG("ERROR: (iotc_set_logging) invalid argument. ERROR:0x0001");
        return 1;
    }
    gLogLevel = level;
    return 0;
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

/* extern */
int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
  const char* device_id, IOTConnectType type) {

    CHECK_NOT_NULL(ctx)
    GET_LENGTH_NOT_NULL_NOT_EMPTY(scope, 256);
    GET_LENGTH_NOT_NULL_NOT_EMPTY(keyORcert, 256);
    GET_LENGTH_NOT_NULL_NOT_EMPTY(device_id, 256);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

    char stringBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    int errorCode = 0;
    size_t pos = 0;
    bool traceOn;

    DevkitDPSSetLogTrace(gLogLevel > IOTC_LOGGING_API_ONLY);
  #ifdef MXCHIP_AZ3166
    // TODO: move it to PAL
    if (type == IOTC_CONNECT_SYMM_KEY)
        DevkitDPSSetAuthType(DPS_AUTH_SYMMETRIC_KEY);
    else if (type == IOTC_CONNECT_X509_CERT)
        DevkitDPSSetAuthType(DPS_AUTH_X509_GROUP);
    else
    {
        IOTC_LOG("ERROR: (iotc_connect) wrong value for IOTConnectType. ERR:0x0001");
        errorCode = 1;
        goto fnc_exit;
    }
  #endif // MXCHIP_AZ3166

    strcpy(stringBuffer, keyORcert);
    if (!DevkitDPSClientStart(internal->endpoint == NULL ? DEFAULT_ENDPOINT : internal->endpoint,
                      scope, device_id, stringBuffer, NULL, 0)) {
        IOTC_LOG("ERROR: (iotc_connect) device registration step has failed. ERR:0x0002");
        errorCode = 2;
        goto fnc_exit;
    } else if (type == IOTC_CONNECT_SYMM_KEY) {
        pos = snprintf(stringBuffer, AZ_IOT_HUB_MAX_LEN,
            "HostName=%s;DeviceId=%s;SharedAccessKey=%s",
            DevkitDPSGetIoTHubURI(),
            DevkitDPSGetDeviceID(),
            keyORcert);
    } else if (type == DPS_AUTH_X509_GROUP) {
        pos = snprintf(stringBuffer, AZ_IOT_HUB_MAX_LEN,
            "HostName=%s;DeviceId=%s;UseProvisioning=true",
            DevkitDPSGetIoTHubURI(),
            DevkitDPSGetDeviceID());
    }

    if (pos == 0 || pos >= AZ_IOT_HUB_MAX_LEN) {
        IOTC_LOG("ERROR: (iotc_connect) connection information is out of buffer. ERR:0x000F");
        errorCode = 15;
        goto fnc_exit;
    }
    stringBuffer[pos] = 0;

    if (platform_init() != 0) {
        IOTC_LOG("ERROR: (iotc_connect) Failed to initialize the SDK platform. ERR:0x0003");
        errorCode = 3;
        goto fnc_exit;
    }

    if ((internal->clientHandle = IoTHubClient_LL_CreateFromConnectionString(
        stringBuffer, MQTT_Protocol)) == NULL) {

        IOTC_LOG("ERROR: (iotc_connect) Couldn't create hub from connection string. ERR:0x0004");
        errorCode = 4;
        goto fnc_exit;
    }

    IoTHubClient_LL_SetRetryPolicy(internal->clientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);
#ifndef MXCHIP_AZ3166
#error "implement certs for non mxchip"
#endif
    if (IoTHubClient_LL_SetOption(internal->clientHandle, "TrustedCerts",
        certificates /* src/cores/arduino/az_iot/azureiotcerts.h */) != IOTHUB_CLIENT_OK) {
        IOTC_LOG("ERROR: Failed to set option \"TrustedCerts\" IoTHubClient_LL_SetOption failed. ERR:0x0005");
        return 5;
    }

    traceOn = gLogLevel > IOTC_LOGGING_API_ONLY;
    IoTHubClient_LL_SetOption(internal->clientHandle, "logtrace", &traceOn);

    if (IoTHubClient_LL_SetMessageCallback(internal->clientHandle, receiveMessageCallback,
        internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005");
        errorCode = 5;
        goto fnc_exit;
    }

    if (IoTHubClient_LL_SetDeviceTwinCallback(internal->clientHandle, deviceTwinGetStateCallback,
        internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005");
        errorCode = 5;
        goto fnc_exit;
    }

    if (IoTHubClient_LL_SetDeviceMethodCallback(internal->clientHandle, onCommand,
        internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005");
        errorCode = 5;
        goto fnc_exit;
    }

    if (IoTHubClient_LL_SetConnectionStatusCallback(internal->clientHandle,
        connectionStatusCallback, internal) != IOTHUB_CLIENT_OK) {

        IOTC_LOG("ERROR: (iotc_connect) IoTHubClient_LL_SetXXXXCallback failed. ERR:0x0005");
        errorCode = 5;
        goto fnc_exit;
    }

fnc_exit:
    return errorCode;
}

/* extern */
int iotc_set_global_endpoint(IOTContext ctx, const char* endpoint_uri) {
    CHECK_NOT_NULL(ctx)
    GET_LENGTH_NOT_NULL_NOT_EMPTY(endpoint_uri, 1024);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

    // todo: do not fragment the memory
    if (internal->endpoint != NULL) {
        free(internal->endpoint);
    }
    internal->endpoint = (char*) malloc(endpoint_uri_len + 1);
    CHECK_NOT_NULL(internal->endpoint);

    strcpy(internal->endpoint, endpoint_uri);
    *(internal->endpoint + endpoint_uri_len) = 0;
    return 0;
}

/* extern */
int iotc_set_protocol(IOTContext ctx, IOTProtocol protocol) {
    CHECK_NOT_NULL(ctx)
    if (protocol < IOTC_PROTOCOL_MQTT || protocol > IOTC_PROTOCOL_HTTP) {
        IOTC_LOG("ERROR: (iotc_set_protocol) invalid argument. ERROR:0x0001");
        return 1;
    }

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

    internal->protocol = protocol;

    return 0;
}

/* extern */
int iotc_set_trusted_certs(IOTContext ctx, const char* certs) {
    CHECK_NOT_NULL(ctx)
    GET_LENGTH_NOT_NULL_NOT_EMPTY(certs, 4096);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    if (IoTHubClient_LL_SetOption(internal->clientHandle, "TrustedCerts",
      certs) != IOTHUB_CLIENT_OK) {

        IOTC_LOG("ERROR: (iotc_set_trusted_certs) IoTHubClient_LL_SetOption for Trusted certs has failed. ERROR:0x0011");
        return 17;
    }

    return 0;
}

/* extern */
int iotc_set_proxy(IOTContext ctx, HTTP_PROXY_OPTIONS proxy) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

    IOTC_LOG("ERROR: (iotc_set_proxy) Not implemented. ERR:0x0008");
    return 8;
}

/* extern */
int iotc_disconnect(IOTContext ctx) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    IOTC_LOG("ERROR: (iotc_disconnect) Not implemented. ERR:0x0008");
    return 8;
}

/* extern */
int iotc_send_telemetry(IOTContext ctx, const char* payload, unsigned length, void* appContext) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;
    int errorCode = 0;
    EVENT_INSTANCE *currentMessage = createEventInstance(internal, payload, length, appContext, &errorCode);
    if (currentMessage == NULL) return errorCode;

    MAP_HANDLE propMap = IoTHubMessage_Properties(currentMessage->messageHandle);

    time_t now_utc = time(NULL); // utc time
    char timeBuffer[128] = {0};
    unsigned outputLength = snprintf(timeBuffer, 128, "%s", ctime(&now_utc));
    assert(outputLength && outputLength < 128 && timeBuffer[outputLength - 1] == '\n');
    timeBuffer[outputLength - 1] = char(0); // replace `\n` with `\0`
    if (Map_AddOrUpdate(propMap, "timestamp", timeBuffer) != MAP_OK)
    {
        IOTC_LOG("ERROR: (iotc_send_telemetry) Map_AddOrUpdate has failed. ERROR:0x000A");
        freeEventInstance(currentMessage);
        return 10;
    }

    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(internal->clientHandle,
        currentMessage->messageHandle, sendConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        IOTC_LOG("ERROR: (iotc_send_telemetry) IoTHubClient_LL_SendEventAsync has "
          "failed => hubResult is (%d). ERROR:0x000B", hubResult);
        freeEventInstance(currentMessage);
        return 11;
    }
    iotc_do_work(ctx);

    return 0;
}

/* extern */
int iotc_send_state    (IOTContext ctx, const char* payload, unsigned length, void* appContext) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    return iotc_send_telemetry((IOTContext)internal, payload, length, appContext);
}

/* extern */
int iotc_send_event    (IOTContext ctx, const char* payload, unsigned length, void* appContext) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    return iotc_send_telemetry((IOTContext)internal, payload, length, appContext);
}

/* extern */
int iotc_send_property (IOTContext ctx, const char* payload, unsigned length, void *appContext) {
    CHECK_NOT_NULL(ctx)
    CHECK_NOT_NULL(payload)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    int errorCode = 0;
    EVENT_INSTANCE *currentMessage = createEventInstance(internal, payload, length, appContext, &errorCode);
    if (currentMessage == NULL) return errorCode;

    IOTHUB_CLIENT_RESULT hubResult = IoTHubClient_LL_SendReportedState(internal->clientHandle,
        (const unsigned char*)payload, length, deviceTwinConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        IOTC_LOG("ERROR: (iotc_send_telemetry) IoTHubClient_LL_SendReportedState has "
          "failed => hubResult is (%d). ERROR:0x000B", hubResult);
        freeEventInstance(currentMessage);
        return 11;
    }
    iotc_do_work(ctx);

    return 0;
}

/* extern */
int iotc_on(IOTContext ctx, const char* eventName, IOTCallback callback, void* appContext) {
    CHECK_NOT_NULL(ctx)
    GET_LENGTH_NOT_NULL_NOT_EMPTY(eventName, 64);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_INIT(internal);

#define SETCB_(x, a, b) x.callback=a;x.appContext=b;
    if (strcmp(eventName, "ConnectionStatus") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::ConnectionStatus], callback, appContext);
    } else if (strcmp(eventName, "MessageSent") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::MessageSent], callback, appContext);
    } else if (strcmp(eventName, "MessageReceived") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::MessageReceived], callback, appContext);
    } else if (strcmp(eventName, "Error") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::Error], callback, appContext);
    } else if (strcmp(eventName, "SettingsUpdated") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::SettingsUpdated], callback, appContext);
    } else if (strcmp(eventName, "Command") == 0) {
        SETCB_(internal->callbacks[IOTCallbacks::Command], callback, appContext);
    } else {
        IOTC_LOG("ERROR: (iotc_on) Unknown event definition. ERROR:0x000E");
        return 14;
    }
#undef SETCB_
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