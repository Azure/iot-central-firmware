// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "Arduino.h"
#include "AzureIotHub.h"

#include <ArduinoJson.h>

#include "../inc/iotCentral.h"
#include "../inc/config.h"
#include "../inc/iotHubClient.h"
#include "../inc/stats.h"
#include "../inc/wifi.h"

#define MAX_CALLBACK_COUNT 32

// forward declarations
static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback);
static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
static int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback);
static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback);
static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback);
static void checkConnection();
void hubClientYield(void);

typedef struct CALLBACK_LOOKUP_TAG {
    char* name;
    methodCallback callback;
} CALLBACK_LOOKUP;

static int callbackCounter;
static int receiveContext = 0;
static int receiveTwinContext = 0;
static int statusContext = 0;
static int trackingId = 0;
static int reconnect = false;
static bool saveTraceOn = false;
static CALLBACK_LOOKUP methodCallbackList[MAX_CALLBACK_COUNT];
static int methodCallbackCount = 0;
static CALLBACK_LOOKUP desiredCallbackList[MAX_CALLBACK_COUNT];
static int desiredCallbackCount = 0;
static String deviceId;
static String hubName;

IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

typedef struct EVENT_INSTANCE_TAG {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    int messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;

void initIotHubClient(bool traceOn) {
    saveTraceOn = traceOn;
    String connString = readConnectionString();;
    deviceId = connString.substring(connString.indexOf("DeviceId=") + 9, connString.indexOf(";SharedAccess"));
    hubName = connString.substring(connString.indexOf("HostName=") + 9, connString.indexOf(";DeviceId="));
    hubName = hubName.substring(0, hubName.indexOf("."));

    if (platform_init() != 0) {
        (void)Serial.printf("Failed to initialize the platform.\r\n");
        return;
    }

    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connString.c_str(), MQTT_Protocol)) == NULL) {
        (void)Serial.printf("ERROR: iotHubClientHandle is NULL!\r\n");
        return;
    }

    IoTHubClient_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);

    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &saveTraceOn);

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("Failed to set option \"TrustedCerts\"\r\n");
        return;
    }

    // Setting Message call back, so we can receive Commands. 
    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
        return;
    }

    // Setting twin call back, so we can receive desired properties. 
    if (IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinGetStateCallback, &receiveTwinContext) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SetDeviceTwinCallback..........FAILED!\r\n");
        return;
    }

    // Twin direct method callback, so we can receive direct method calls
    if (IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, DeviceDirectMethodCallback, &receiveContext) != IOTHUB_CLIENT_OK) {
        (void)printf("ERROR: IoTHubClient_LL_SetDeviceMethodCallback..........FAILED!\r\n");
        return;
    }

    // Connection status change callback
    if (IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle, connectionStatusCallback, &statusContext) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SetConnectionStatusCallback..........FAILED!\r\n");
        return;
    }
}

bool sendTelemetry(const char *payload) {
    checkConnection();
    
    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;

    EVENT_INSTANCE *currentMessage = (EVENT_INSTANCE*)malloc(sizeof(EVENT_INSTANCE));
    currentMessage->messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, strlen(payload));
    if (currentMessage->messageHandle == NULL) {
        (void)Serial.printf("ERROR: iotHubMessageHandle is NULL!\r\n");
        free(currentMessage);
        return false;
    }
    currentMessage->messageTrackingId = trackingId++;

    MAP_HANDLE propMap = IoTHubMessage_Properties(currentMessage->messageHandle);

    // add a timestamp to the message - illustrated for the use in batching
    time_t seconds = time(NULL);
    String temp = ctime(&seconds);
    temp.replace("\n","\0");
    if (Map_AddOrUpdate(propMap, "timestamp", temp.c_str()) != MAP_OK)
    {
        Serial.println("ERROR: Adding message property failed");
    }
    
    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(iotHubClientHandle, currentMessage->messageHandle, sendConfirmationCallback, currentMessage);
    if (hubResult != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED (%s)!\r\n", hubResult);
        incrementErrorCount();
        IoTHubMessage_Destroy(currentMessage->messageHandle);
        free(currentMessage);
        return false;
    }
    (void)Serial.printf("IoTHubClient_LL_SendEventAsync accepted message for transmission to IoT Hub.\r\n");

    // yield to process any work to/from the hub
    hubClientYield();

    return true;
}

bool sendReportedProperty(const char *payload) {
    checkConnection();
    
    bool retValue = true;
    
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(iotHubClientHandle, (const unsigned char*)payload, strlen(payload), deviceTwinConfirmationCallback, NULL);

    if (result != IOTHUB_CLIENT_OK) {
        LogError("Failure sending reported property!!!");
        retValue = false;
    }

    return retValue;
}

// register callbacks for direct and cloud to device messages
bool registerMethod(const char *methodName, methodCallback callback) {
    if (methodCallbackCount < MAX_CALLBACK_COUNT) {
        methodCallbackList[methodCallbackCount].name = (char*)malloc(strlen(methodName) + 1);
        strcpy(methodCallbackList[methodCallbackCount].name, (char*)methodName);
        methodCallbackList[methodCallbackCount].name = strupr(methodCallbackList[methodCallbackCount].name);
        methodCallbackList[methodCallbackCount].callback = callback;
        methodCallbackCount++;
        return true;
    } else {
        return false;
    }
}

// register callbacks for desired properties
bool registerDesiredProperty(const char *propertyName, methodCallback callback) {
    if (desiredCallbackCount < MAX_CALLBACK_COUNT) {
        desiredCallbackList[desiredCallbackCount].name = (char*)malloc(strlen(propertyName) + 1);
        strcpy(desiredCallbackList[desiredCallbackCount].name, (char*)propertyName);
        desiredCallbackList[desiredCallbackCount].name = strupr(desiredCallbackList[desiredCallbackCount].name);
        desiredCallbackList[desiredCallbackCount].callback = callback;
        desiredCallbackCount++;
        return true;
    } else {
        return false;
    }
}

void closeIotHubClient(void)
{
    IoTHubClient_LL_Destroy(iotHubClientHandle);
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
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

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(buffer);

    String methodName = root["methodName"];
    methodName.toUpperCase();

    int status = 0;
    char *methodResponse;
    size_t responseSize;

    // lookup if the method has been registered to a function
    for(int i = 0; i < methodCallbackCount; i++) {
        if (methodName == methodCallbackList[i].name) {
            String params = root["payload"];
            status = methodCallbackList[i].callback((const char*)params.c_str(), params.length(), &methodResponse, &responseSize);
            break;
        }
    }

    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    (void)userContextCallback;

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

    Serial.printf("Device Method %s called\r\n"), method_name;

    String responseString = "{ \"Response\":{{response}}}";
    responseString.replace("{{response}}", methodResponse);

    *resp_size = responseString.length();
    if ((*response = (unsigned char*)malloc(*resp_size)) == NULL) {
        status = -1;
    } else {
        responseString.toCharArray((char*)*response, *resp_size + 1);
    }

    return status;
}

// every desired property change gets echoed back as a reported property
void echoDesired(const char *propertyName, const char *message, const char *status, int statusCode) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(message);
    const char *value = root[propertyName]["value"];
    const char *desiredVersion = root["$version"];
    if (root.containsKey("desired")) {
        value = root["desired"][propertyName]["value"];
        desiredVersion = root["desired"]["$version"];   
    } else {
        propertyName = root.begin()->key;
        value = root[propertyName]["value"];
        desiredVersion = root["$version"];     
    }

    char buff[4096];
    String echoTemplate = F("{\"%s\":{\"value\":%s, \"statusCode\":%d, \"status\":\"%s\", \"desiredVersion\":%s}}");
    sprintf(buff, echoTemplate.c_str(), propertyName, value, statusCode, status, desiredVersion);
    Serial.printf(buff);

    if (sendReportedProperty(buff)) {
        Serial.printf("Desired property %s successfully echoed back as a reported property", propertyName);
        incrementReportedCount();
    } else {
        Serial.printf("Desired property %s failed to be echoed back as a reported property", propertyName);
        incrementErrorCount();
    }
}

void callDesiredCallback(const char *propertyName, const char *payLoad, size_t size) {
    const char *propName = strdup(propertyName);
    propName = strupr((char*)propName);
    int status = 0;
    char *methodResponse;
    size_t responseSize;

    for(int i = 0; i < desiredCallbackCount; i++) {
        if (strcmp(propName, desiredCallbackList[i].name) == 0) {
            status = desiredCallbackList[i].callback(payLoad, size, &methodResponse, &responseSize);
            echoDesired(propertyName, payLoad, methodResponse, status);
            free(methodResponse);
            break;
        }
    }
}

static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback) {
    ((char*)payLoad)[size] = 0x00;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payLoad);

    if (update_state == DEVICE_TWIN_UPDATE_PARTIAL) {
        callDesiredCallback(root.begin()->key, (const char*)payLoad, size);
    } else {
        // loop through all the desired properties
        // look to see if the desired property has an associated reported property
        // if so look if the versions match, if they match do nothing
        // if they don't match then call the associated callback for the desired property

        Serial.println("Processing complete twin");

        JsonObject& desired = root["desired"];
        JsonObject& reported = root["reported"];

        for (JsonObject::iterator it=desired.begin(); it!=desired.end(); ++it) {
            if (it->key[0] != '$') {
                if (reported.containsKey(it->key) && (reported[it->key]["desiredVersion"] == desired["$version"])) {
                    Serial.printf("key: %s found in reported and versions match\r\n", it->key);
                } else if (reported[it->key]["value"] != desired[it->key]["value"]){
                    Serial.printf("key: %s either not found in reported or versions do not match\r\n", it->key);
                    Serial.println(it->value.as<char*>());
                    callDesiredCallback(it->key, (const char*)payLoad, size);
                } else {
                    echoDesired(it->key, (const char*)payLoad, "completed", 200);
                }
            }
        }
    }
}

static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback) {
    (void)(userContextCallback);
    LogInfo("DeviceTwin CallBack: Status_code = %u", status_code);
}

static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {
    if (reason == IOTHUB_CLIENT_CONNECTION_NO_NETWORK) {
        (void)Serial.println("No network connection");
        reconnect = true;      
    } else if (result == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED && reason == IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN) {
        (void)Serial.println("Connection timeout");
        reconnect = true;
    }
}

static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;
    (void)Serial.printf("Confirmation[%d] received for message tracking id = %d with result = %s\r\n", callbackCounter, eventInstance->messageTrackingId, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
    free(eventInstance);
}

static void checkConnection() {
    if (reconnect) {
        // simple reconnection of the client in the event of a disconnect
        Serial.println("Reconnecting to the IoT Hub");
        closeIotHubClient();
        initIotHubClient(saveTraceOn);

        reconnect = false;
    }
}

void hubClientYield(void) {
    checkConnection();
    
    int waitTime = 1;
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    ThreadAPI_Sleep(waitTime);
}

// scrolling text global variables
int displayCharPos = 0; 
int waitCount = 3;
String displayHubName;

void displayDeviceInfo() {
    char buff[64]; 
    // code to scroll the larger hubname if it exceeds 16 characters
    if (waitCount >= 3) {
        waitCount = 0;
        if (hubName.length() > 16) {
            displayHubName = hubName.substring(displayCharPos);
            if (displayCharPos + 16 >= hubName.length())
                displayCharPos = -1;
            displayCharPos++;
        } else {
            displayHubName = hubName;
        }
    } else {
        waitCount++;
    }

    sprintf(buff, "Device:\r\n%s\r\n%.16s\r\nf/w: %s", deviceId.c_str(), displayHubName.c_str(), FW_VERSION);
    Screen.print(0, buff);
}
