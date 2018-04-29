// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/iotHubClient.h"
#include <time.h>

#include "../inc/utility.h"
#include "../inc/config.h"

#include "../inc/stats.h"
#include "../inc/wifi.h"

// forward declarations
static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback);
static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
static int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback);
static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback);
static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback);

static int methodCallbackCount = 0;
static int desiredCallbackCount = 0;
static CALLBACK_LOOKUP methodCallbackList[MAX_CALLBACK_COUNT];
static CALLBACK_LOOKUP desiredCallbackList[MAX_CALLBACK_COUNT];

static int statusContext = 0;
static int trackingId = 0;

void IoTHubClient::initIotHubClient() {
    char connectionString[AZ_IOT_HUB_MAX_LEN] = {0};
    readConnectionString(connectionString, AZ_IOT_HUB_MAX_LEN);

    String connString(connectionString);
    String deviceIdString = connString.substring(connString.indexOf("DeviceId=")
                            + 9, connString.indexOf(";SharedAccess"));

    assert(deviceIdString.length() < IOT_CENTRAL_MAX_LEN);
    memcpy(deviceId, deviceIdString.c_str(), deviceIdString.length());

    String hubNameString = connString.substring(connString.indexOf("HostName=")
                            + 9, connString.indexOf(";DeviceId="));
    hubNameString = hubNameString.substring(0, hubNameString.indexOf("."));

    assert(hubNameString.length() < IOT_CENTRAL_MAX_LEN);
    memcpy(hubName, hubNameString.c_str(), hubNameString.length());

    if (platform_init() != 0) {
        Serial.println("Failed to initialize the platform.");
        hasError = true;
        return;
    }

    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(
        connString.c_str(), MQTT_Protocol)) == NULL) {

        Serial.println("ERROR: iotHubClientHandle is NULL!");
        hasError = true;
        return;
    }

    IoTHubClient_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);
    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts",
        certificates /* src/cores/arduino/az_iot/azureiotcerts.h */) != IOTHUB_CLIENT_OK) {

        Serial.println("Failed to set option \"TrustedCerts\"");
        hasError = true;
        return;
    }

    int receiveContext = 0;
    // Setting Message call back, so we can receive Commands.
    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback,
        &receiveContext) != IOTHUB_CLIENT_OK) {

        Serial.println("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!");
        hasError = true;
        return;
    }

    int receiveTwinContext = 0;
    // Setting twin call back, so we can receive desired properties.
    if (IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinGetStateCallback,
        &receiveTwinContext) != IOTHUB_CLIENT_OK) {

        Serial.println("ERROR: IoTHubClient_LL_SetDeviceTwinCallback..........FAILED!");
        hasError = true;
        return;
    }

    // Twin direct method callback, so we can receive direct method calls
    if (IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, DeviceDirectMethodCallback,
        &receiveContext) != IOTHUB_CLIENT_OK) {

        Serial.println("ERROR: IoTHubClient_LL_SetDeviceMethodCallback..........FAILED!");
        hasError = true;
        return;
    }

    // Connection status change callback
    if (IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle,
        connectionStatusCallback, &statusContext) != IOTHUB_CLIENT_OK) {

        Serial.println("ERROR: IoTHubClient_LL_SetConnectionStatusCallback..........FAILED!");
        hasError = true;
        return;
    }
}

bool IoTHubClient::sendTelemetry(const char *payload) {
    checkConnection();

    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;
    EVENT_INSTANCE *currentMessage = (EVENT_INSTANCE*) malloc(sizeof(EVENT_INSTANCE));
    currentMessage->messageHandle =
        IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, strlen(payload));

    if (currentMessage->messageHandle == NULL) {
        Serial.println("ERROR: iotHubMessageHandle is NULL!");
        free(currentMessage);
        return false;
    }
    currentMessage->messageTrackingId = trackingId++;

    MAP_HANDLE propMap = IoTHubMessage_Properties(currentMessage->messageHandle);

    // add a timestamp to the message - illustrated for the use in batching
    time_t now_utc = time(NULL); // utc time
    char timeBuffer[STRING_BUFFER_128] = {0};
    unsigned outputLength = snprintf(timeBuffer, STRING_BUFFER_128, "%s", ctime(&now_utc));
    assert(outputLength && outputLength < STRING_BUFFER_128 && timeBuffer[outputLength - 1] == '\n');
    timeBuffer[outputLength - 1] = char(0); // replace `\n` with `\0`
    if (Map_AddOrUpdate(propMap, "timestamp", timeBuffer) != MAP_OK)
    {
        Serial.println("ERROR: Adding message property failed");
    }

    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(iotHubClientHandle,
        currentMessage->messageHandle, sendConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        Serial.printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED hubResult is (%d)!\r\n", hubResult);
        incrementErrorCount();
        IoTHubMessage_Destroy(currentMessage->messageHandle);
        free(currentMessage);
        return false;
    }

    // keep happy messages small (serial monitor is failing for latter errors)
    Serial.println("IoTHubClient_LL_SendEventAsync [CHECK]");

    // yield to process any work to/from the hub
    hubClientYield();

    return true;
}

bool IoTHubClient::sendReportedProperty(const char *payload) {
    checkConnection();

    bool retValue = true;

    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(iotHubClientHandle,
        (const unsigned char*)payload, strlen(payload), deviceTwinConfirmationCallback, NULL);

    if (result != IOTHUB_CLIENT_OK) {
        LogError("Failure sending reported property!!!");
        retValue = false;
    }

    return retValue;
}

// register callbacks for direct and cloud to device messages
bool IoTHubClient::registerMethod(const char *methodName, hubMethodCallback callback) {
    if (methodCallbackCount < MAX_CALLBACK_COUNT) {
        const uint32_t methodNameLength = strlen(methodName);
        methodCallbackList[methodCallbackCount].name = (char*) malloc(methodNameLength + 1);
        strncpy(methodCallbackList[methodCallbackCount].name, methodName, methodNameLength);
        methodCallbackList[methodCallbackCount].name[methodNameLength] = char(0);
        strupr(methodCallbackList[methodCallbackCount].name);
        methodCallbackList[methodCallbackCount].callback = callback;
        methodCallbackCount++;
        return true;
    }

    return false;
}

// register callbacks for desired properties
bool IoTHubClient::registerDesiredProperty(const char *propertyName, hubMethodCallback callback) {
    if (desiredCallbackCount < MAX_CALLBACK_COUNT) {
        const uint32_t propertyNameLength = strlen(propertyName);
        desiredCallbackList[desiredCallbackCount].name = (char*) malloc(propertyNameLength + 1);
        strncpy(desiredCallbackList[desiredCallbackCount].name, (char*)propertyName, propertyNameLength);
        desiredCallbackList[desiredCallbackCount].name[propertyNameLength] = char(0);
        strupr(desiredCallbackList[desiredCallbackCount].name);
        desiredCallbackList[desiredCallbackCount].callback = callback;
        desiredCallbackCount++;
        return true;
    }

    return false;
}

void IoTHubClient::closeIotHubClient()
{
    IoTHubClient_LL_Destroy(iotHubClientHandle);
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback
    (IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    int *counter = (int *)userContextCallback;
    const char *buffer;
    size_t size;

    // get message content
    if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer, &size) != IOTHUB_MESSAGE_OK) {
        (void)Serial.printf("unable to retrieve the message data\r\n");
    } else {
        (void)Serial.printf("Received Message [%d], Size=%d\r\n", *counter, (int)size);
    }

    // message format expected:
    // {
    //     "methodName" : "<method name>",
    //     "payload" : {
    //         "input1": "someInput",
    //         "input2": "anotherInput"
    //         ...
    //     }
    // }

    JSObject json(buffer);
    String methodName = json.getStringByName("methodName");
    if (methodName.length() == 0) {
        LOG_ERROR("Object doesn't have a member 'methodName'");
        return IOTHUBMESSAGE_REJECTED;
    }
    methodName.toUpperCase();

    char *methodResponse;
    size_t responseSize;

    // lookup if the method has been registered to a function
    for(int i = 0; i < methodCallbackCount; i++) {
        if (methodName == methodCallbackList[i].name) {
            const char* params = json.getStringByName("payload");
            if (params == NULL) {
                LOG_ERROR("Object doesn't have a member 'payload'");
                return IOTHUBMESSAGE_REJECTED;
            }

            methodCallbackList[i].callback(params, strlen(params),
                &methodResponse, &responseSize);
            break;
        }
    }

    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload,
    size_t size, unsigned char** response, size_t* resp_size, void* /* userContextCallback */)
{
    // message format expected:
    // {
    //     "methodName": "reboot",
    //     "responseTimeoutInSeconds": 200,
    //     "payload": {
    //         "input1": "someInput",
    //         "input2": "anotherInput"
    //         ...
    //     }
    // }

    int status = 0;
    char* methodResponse;
    size_t responseSize;

    // lookup if the method has been registered to a function
    String methodName = method_name;
    methodName.toUpperCase();
    for(int i = 0; i < methodCallbackCount; i++) {
        if (methodName == methodCallbackList[i].name) {
            status = methodCallbackList[i].callback((const char*)payload, size, &methodResponse, &responseSize);
            break;
        }
    }

    Serial.printf("Device Method %s called\r\n", method_name);

    const char * message_template = "{\"Response\":%s}";
    const int template_size = strlen(message_template) - 2 /* %s */;
    uint32_t message_size = responseSize + template_size;
    char *tmp_string = (char*) malloc(message_size + 1);
    if (tmp_string == NULL) {
        status = -1;
    } else {
        snprintf(tmp_string, message_size, message_template, methodResponse);
        tmp_string[message_size] = char(0);
        *response = reinterpret_cast<unsigned char*>(tmp_string);
        *resp_size = message_size;
    }

    return status;
}

// every desired property change gets echoed back as a reported property
void echoDesired(const char *propertyName, const char *message, const char *status, int statusCode) {
    JSObject rootObject(message);
    JSObject propertyNameObject, desiredObject, desiredObjectPropertyName;

    const char* methodName = rootObject.getStringByName("methodName");
    if (methodName == NULL) {
        LOG_ERROR("Object doesn't have a member 'methodName'");
        return;
    }

    if (!rootObject.getObjectByName(propertyName, &propertyNameObject)) {
        LOG_ERROR("Object doesn't have a member with propertyName");
        return;
    }

    const char *value, *desiredVersion;
    if (rootObject.getObjectByName("desired", &desiredObject)) {
        if (!desiredObject.getObjectByName(propertyName, &desiredObjectPropertyName)) {
            LOG_ERROR("desiredObject doesn't have a member with propertyName");
            return;
        }

        value = desiredObjectPropertyName.getStringByName("value");
        desiredVersion = desiredObject.getStringByName("$version");
    } else {
        propertyName = rootObject.getNameAt(0);

        value = propertyNameObject.getStringByName("value");
        desiredVersion = rootObject.getStringByName("$version");
    }

    char buff[STRING_BUFFER_1024] = {0};
    char *tmp = buff;
    const char* echoTemplate = "{\"%s\":{\"value\":%s, \"statusCode\":%d, \"status\":\"%s\", \"desiredVersion\":%s}}";
    uint32_t buffer_size = snprintf(NULL, 0, echoTemplate, propertyName, value, statusCode, status, desiredVersion);
    if (buffer_size + 1 > STRING_BUFFER_1024) {
        tmp = (char*) malloc(buffer_size + 1);
        if (tmp == NULL) {
            Serial.printf("Desired property %s failed to be echoed back as a reported property (OUT OF MEMORY)\r\n", propertyName);
            incrementErrorCount();
            return;
        }
    }

    snprintf(tmp, buffer_size, echoTemplate, propertyName, value, statusCode, status, desiredVersion);
    buff[buffer_size] = char(0);
    Serial.printf(buff);

    if (Globals::iothubClient->sendReportedProperty(buff)) {
        Serial.printf("Desired property %s successfully echoed back as a reported property\r\n", propertyName);
        incrementReportedCount();
    } else {
        Serial.printf("Desired property %s failed to be echoed back as a reported property\r\n", propertyName);
        incrementErrorCount();
    }

    if (buff != tmp) {
        free(tmp);
    }
}

void callDesiredCallback(const char *propertyName, const char *payLoad, size_t size) {
    int status = 0;
    char *methodResponse;
    size_t responseSize;
    char *propName = strdup(propertyName);
    strupr(propName);

    for(int i = 0; i < desiredCallbackCount; i++) {
        if (strcmp(propName, desiredCallbackList[i].name) == 0) {
            status = desiredCallbackList[i].callback(payLoad, size, &methodResponse, &responseSize);
            echoDesired(propertyName, payLoad, methodResponse, status);
            free(methodResponse);
            break;
        }
    }
    free(propName);
}

void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state,
    const unsigned char* payLoad, size_t size, void* userContextCallback) {

    ((char*)payLoad)[size] = 0x00;
    JSObject payloadObject((const char *)payLoad);

    if (update_state == DEVICE_TWIN_UPDATE_PARTIAL) {
        callDesiredCallback(payloadObject.getNameAt(0), reinterpret_cast<const char*>(payLoad), size);
    } else {
        JSObject desired, reported;

        // loop through all the desired properties
        // look to see if the desired property has an associated reported property
        // if so look if the versions match, if they match do nothing
        // if they don't match then call the associated callback for the desired property

        Serial.println("Processing complete twin");

        payloadObject.getObjectByName("desired", &desired);
        payloadObject.getObjectByName("reported", &reported);

        for (unsigned i = 0, count = desired.getCount(); i < count; i++) {
            JSObject item;
            const char * itemName = desired.getNameAt(i);
            if (itemName[0] != '$') {
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
                    Serial.printf("key: %s found in reported and versions match\r\n", itemName);
                } else if (containsKey && strcmp(desiredValue, value) != 0){
                    Serial.printf("key: %s either not found in reported or versions do not match\r\n", itemName);
                    JSObject itemValue;
                    if (desired.getObjectAt(i, &itemValue) && itemValue.toString() != NULL) {
                        Serial.println(itemValue.toString());
                    } else {
                        LOG_ERROR("desired doesn't have value at index");
                    }
                    callDesiredCallback(itemName, (const char*)payLoad, size);
                } else {
                    echoDesired(itemName, (const char*)payLoad, Globals::completedString, 200);
                }
            }
        }
    }
}

static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback) {
    assert(userContextCallback == NULL); // NOOP for now, so it should be NULL
    LogInfo("DeviceTwin CallBack: Status_code = %u", status_code);
}

static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {

    if (reason == IOTHUB_CLIENT_CONNECTION_NO_NETWORK) {
        Serial.println("No network connection");
        Globals::iothubClient->needsReconnect = true;
    } else if (result == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED &&
               reason == IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN) {
        Serial.println("Connection timeout");
        Globals::iothubClient->needsReconnect = true;
    }
}

static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
    static int callbackCounter = 0;
    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;
    Serial.printf("Confirmation[%d] received for message tracking id = %d \
        with result = %s\r\n", callbackCounter++, eventInstance->messageTrackingId,
        ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));

    IoTHubMessage_Destroy(eventInstance->messageHandle);
    free(eventInstance);
}

// scrolling text global variables
void IoTHubClient::displayDeviceInfo() {
    char buff[STRING_BUFFER_128] = {0};

    if (needsCopying) {
        // code to scroll the larger hubname if it exceeds 16 characters
        if (waitCount >= 3) {
            waitCount = 0;
            const size_t hubNameLength = strlen(hubName);
            if (hubNameLength > AZ3166_DISPLAY_MAX_COLUMN) {
                unsigned length = min(hubNameLength - displayCharPos, AZ3166_DISPLAY_MAX_COLUMN);
                memcpy(displayHubName, hubName + displayCharPos, length);
                displayHubName[length] = char(0); // future proof
                if ((size_t) AZ3166_DISPLAY_MAX_COLUMN + displayCharPos >= hubNameLength) {
                    displayCharPos = 0;
                } else {
                    displayCharPos++;
                }
            } else { // hubNameLength > AZ3166_DISPLAY_MAX_COLUMN
                memcpy(displayHubName, hubName, hubNameLength);
                displayHubName[hubNameLength] = char(0); // future proof
                needsCopying = false; // no need to do this again
            } // hubNameLength > AZ3166_DISPLAY_MAX_COLUMN
        } else {
            waitCount++;
        } // waitCount >= 3
    } // needsCopying

    snprintf(buff, STRING_BUFFER_128 - 1, "Device:\r\n%s\r\n%.16s\r\nf/w: %s",
        deviceId, displayHubName, AZIOTC_FW_VERSION);
    Screen.print(0, buff);
}
