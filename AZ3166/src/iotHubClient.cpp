// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/iotHubClient.h"
#include <time.h>

#include "../inc/telemetry.h"
#include "../inc/utility.h"
#include "../inc/config.h"

#include "../inc/stats.h"
#include "../inc/wifi.h"

#include "../inc/watchdogController.h"
#include <DevkitDPSClient.h>

// forward declarations
static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback);
static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
static int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback);
static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback);
static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback);

void IoTHubClient::initIotHubClient() {
    LOG_VERBOSE("IoTHubClient::initIotHubClient START");
    char stringBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    char scopeId[STRING_BUFFER_128] = {0};
    char registrationId[STRING_BUFFER_128] = {0};
    bool sasKey = false;
    ConfigController::readGroupSXKeyAndDeviceId(scopeId, registrationId, stringBuffer, sasKey);
    LOG_VERBOSE("SCOPEID: %s REGID: %s KEY: %s IS_SAS: %d", scopeId, registrationId, stringBuffer, sasKey);

    DevkitDPSSetLogTrace(SERIAL_VERBOSE_LOGGING_ENABLED);
    DevkitDPSSetAuthType(sasKey ? DPS_AUTH_SYMMETRIC_KEY : DPS_AUTH_X509_GROUP);
    if (!DevkitDPSClientStart("global.azure-devices-provisioning.net",
                        scopeId, registrationId, stringBuffer, NULL, 0)) {
        LOG_ERROR("ERROR: DPS client for GroupSAS has failed.");
        hasError = true;
        return;
    } else {
        char newConnectionString[AZ_IOT_HUB_MAX_LEN] = {0};
        size_t pos = snprintf(newConnectionString, AZ_IOT_HUB_MAX_LEN,
            "HostName=%s;DeviceId=%s;SharedAccessKey=%s",
            DevkitDPSGetIoTHubURI(),
            DevkitDPSGetDeviceID(),
            stringBuffer);
        assert(pos < AZ_IOT_HUB_MAX_LEN);
        WatchdogController::reset();
        strncpy(stringBuffer, newConnectionString, pos);
        stringBuffer[pos] = char(0);
    }
    // Previously;
    // ConfigController::readConnectionString(stringBuffer, AZ_IOT_HUB_MAX_LEN);

    String connString(stringBuffer);
    String deviceIdString = connString.substring(connString.indexOf("DeviceId=")
                            + 9, connString.indexOf(";SharedAccess"));

    if (deviceIdString.length() >= IOT_CENTRAL_MAX_LEN) {
        LOG_ERROR("deviceIdString.length() >= IOT_CENTRAL_MAX_LEN...\n \
deviceIdString : %s\n \
connString: %s", deviceIdString.c_str(), connString.c_str());
        return;
    }
    memcpy(deviceId, deviceIdString.c_str(), deviceIdString.length());

    String hubNameString = connString.substring(connString.indexOf("HostName=")
                            + 9, connString.indexOf(";DeviceId="));
    hubNameString = hubNameString.substring(0, hubNameString.indexOf("."));

    assert(hubNameString.length() < IOT_CENTRAL_MAX_LEN);
    memcpy(hubName, hubNameString.c_str(), hubNameString.length());

    if (platform_init() != 0) {
        LOG_ERROR("Failed to initialize the platform.");
        hasError = true;
        return;
    }

    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(
        connString.c_str(), MQTT_Protocol)) == NULL) {

        LOG_ERROR("ERROR: iotHubClientHandle is NULL!");
        hasError = true;
        return;
    }

    IoTHubClient_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);
    bool traceOn = IOTHUB_TRACE_LOG_ENABLED;
    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts",
        certificates /* src/cores/arduino/az_iot/azureiotcerts.h */) != IOTHUB_CLIENT_OK) {

        LOG_ERROR("Failed to set option \"TrustedCerts\"");
        hasError = true;
        return;
    }

    int receiveContext = 0;
    // Setting Message call back, so we can receive Commands.
    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback,
        &receiveContext) != IOTHUB_CLIENT_OK) {

        LOG_ERROR("IoTHubClient_LL_SetMessageCallback..........FAILED!");
        hasError = true;
        return;
    }

    int receiveTwinContext = 0;
    // Setting twin call back, so we can receive desired properties.
    if (IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinGetStateCallback,
        &receiveTwinContext) != IOTHUB_CLIENT_OK) {

        LOG_ERROR("IoTHubClient_LL_SetDeviceTwinCallback..........FAILED!");
        hasError = true;
        return;
    }

    // Twin direct method callback, so we can receive direct method calls
    if (IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, DeviceDirectMethodCallback,
        &receiveContext) != IOTHUB_CLIENT_OK) {

        LOG_ERROR("IoTHubClient_LL_SetDeviceMethodCallback..........FAILED!");
        hasError = true;
        return;
    }

    int statusContext = 0; // noop
    // Connection status change callback
    if (IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle,
        connectionStatusCallback, &statusContext) != IOTHUB_CLIENT_OK) {

        LOG_ERROR("IoTHubClient_LL_SetConnectionStatusCallback..........FAILED!");
        hasError = true;
        return;
    }

    LOG_VERBOSE("IoTHubClient::initIotHubClient END");
}

bool IoTHubClient::sendTelemetry(const char *payload) {
    checkConnection();

    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;
    EVENT_INSTANCE *currentMessage = (EVENT_INSTANCE*) malloc(sizeof(EVENT_INSTANCE));
    currentMessage->messageHandle =
        IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, strlen(payload));

    if (currentMessage->messageHandle == NULL) {
        LOG_ERROR("iotHubMessageHandle is NULL!");
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
        LOG_ERROR("Adding message property failed");
    }

    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(iotHubClientHandle,
        currentMessage->messageHandle, sendConfirmationCallback, currentMessage);

    if (hubResult != IOTHUB_CLIENT_OK) {
        LOG_ERROR("IoTHubClient_LL_SendEventAsync..........FAILED hubResult is (%d)", hubResult);
        StatsController::incrementErrorCount();
        IoTHubMessage_Destroy(currentMessage->messageHandle);
        free(currentMessage);
        return false;
    }

    LOG_VERBOSE("IoTHubClient::sendTelemetry COMPLETED");
    return true;
}

bool IoTHubClient::sendReportedProperty(const char *payload) {
    checkConnection();

    bool retValue = true;

    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(iotHubClientHandle,
        (const unsigned char*)payload, strlen(payload), deviceTwinConfirmationCallback, NULL);

    if (result != IOTHUB_CLIENT_OK) {
        LOG_ERROR("Failure sending reported property!!!");
        retValue = false;
    }
    hubClientYield();

    LOG_VERBOSE("IoTHubClient::sendReportedProperty COMPLETED");
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
bool IoTHubClient::registerDesiredProperty(const char *propertyName, hubDesiredCallback callback) {
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
    if (iotHubClientHandle != NULL) {
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        iotHubClientHandle = NULL;
    }
    LOG_ERROR("IoTHubClient::closeIotHubClient!");
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback
    (IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    LOG_VERBOSE("IoTHubClient::receiveMessageCallback (%s)", message);

    int *counter = (int *)userContextCallback;
    const char *buffer;
    size_t size;

    // get message content
    if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer, &size) != IOTHUB_MESSAGE_OK) {
        LOG_ERROR("- IoTHubMessage_GetByteArray: unable to retrieve the message data");
    } else {
        LOG_VERBOSE("Received Message [%d], Size=%lu", *counter, size);
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

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*)Globals::loopController;
    }

    IOTHUBMESSAGE_DISPOSITION_RESULT result = IOTHUBMESSAGE_REJECTED;
    if (telemetryController != NULL && telemetryController->getHubClient() != NULL) {
        IoTHubClient *hubClient = telemetryController->getHubClient();
        // lookup if the method has been registered to a function
        for(int i = 0; i < hubClient->methodCallbackCount; i++) {
            if (methodName == hubClient->methodCallbackList[i].name) {
                const char* payload = json.getStringByName("payload");
                if (payload == NULL) {
                    LOG_ERROR("Object doesn't have a member 'payload'");
                    return result;
                }

                char *pcopy = strdup((const char*)payload);
                assert(pcopy);
                char *mcopy = strdup(methodName.c_str());
                assert(mcopy);

                hubClient->pushDirectMethod(mcopy, pcopy, size);
                result = IOTHUBMESSAGE_ACCEPTED;
                break;
            }
        }

        (*counter)++;
    }

    return result;
}

int DeviceDirectMethodCallback(const char* method_name, const unsigned char* payload,
    size_t size, unsigned char** response, size_t* resp_size, void* /* userContextCallback */)
{
    LOG_VERBOSE("IoTHubClient::DeviceDirectMethodCallback (%s)", method_name);
    int status = 0;
    char* methodResponse;
    size_t responseSize;

    assert(response != NULL && resp_size != NULL);
    *response = NULL;
    *resp_size = 0;
    WatchdogController::reset();

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*)Globals::loopController;
    }

    if (telemetryController != NULL && telemetryController->getHubClient() != NULL) {
        // lookup if the method has been registered to a function
        IoTHubClient *hubClient = telemetryController->getHubClient();
        String methodName = method_name;
        methodName.toUpperCase();
        for(int i = 0; i < hubClient->methodCallbackCount; i++) {
            if (methodName == hubClient->methodCallbackList[i].name) {
                status = 200; // accept the call
                AutoString payloadCopy((const char*)payload, size); // payload may not be null ended
                payloadCopy.makePersistent();
                char *pcopy = *payloadCopy;
                assert(pcopy);
                char *mcopy = strdup(methodName.c_str());
                assert(mcopy);

                hubClient->pushDirectMethod(mcopy, pcopy, size);
                break;
            }
        }

        AutoString responseString(3);
        responseString.makePersistent();
        sprintf(*responseString, EMPTY_JSON);
        (*responseString)[2] = 0;
        *response = (unsigned char*)*responseString;
        *resp_size = 2;
    }

    return status;
}

// every desired property change gets echoed back as a reported property
void echoDesired(const char *propertyName, const char *message, const char *status, int statusCode) {
    LOG_VERBOSE("IoTHubClient::echoDesired (%s)", message);
    JSObject rootObject(message);
    JSObject propertyNameObject, desiredObject, desiredObjectPropertyName;
    LOG_VERBOSE("echoDesired is received - pn: %s m: %s s: %s sc: %d", propertyName, message, status, statusCode);
    const char* methodName = rootObject.getStringByName("methodName");
    if (methodName == NULL) {
        LOG_VERBOSE("Object doesn't have a member 'methodName'");
    }

    if (!rootObject.getObjectByName(propertyName, &propertyNameObject)) {
        LOG_VERBOSE("Object doesn't have a member with propertyName");
    }

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

    AutoString buffer(buffer_size + 1); // +1 is a weird snprintf bug on arduino. needs more investigation

    if (buffer.getLength() == 0) {
        LOG_ERROR("Desired property %s failed to be echoed back as a reported \
            property (OUT OF MEMORY)", propertyName);
        StatsController::incrementErrorCount();
        return;
    }

    snprintf(*buffer, buffer_size + 1, echoTemplate, propertyName,
             (int) value, statusCode,
             status, (int) desiredVersion);
    LOG_VERBOSE("Sending reported property buffer: %s", *buffer);

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*)Globals::loopController;
    }

    if (telemetryController != NULL && telemetryController->getHubClient() != NULL &&
        telemetryController->getHubClient()->sendReportedProperty(*buffer)) {
        LOG_VERBOSE("Desired property %s successfully echoed back as a reported property.", propertyName);
        StatsController::incrementReportedCount();
    } else {
        LOG_ERROR("Desired property %s failed to be echoed back as a reported property.", propertyName);
        StatsController::incrementErrorCount();
    }
}

void callDesiredCallback(const char *propertyName, const char *payLoad, size_t size) {
    LOG_VERBOSE("IoTHubClient::callDesiredCallback (%s)", payLoad);
    int status = 0;
    char *methodResponse;
    size_t responseSize;

    WatchdogController::reset();
    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*)Globals::loopController;
    }

    if (telemetryController != NULL && telemetryController->getHubClient() != NULL) {
        IoTHubClient *hubClient = telemetryController->getHubClient();
        AutoString propName(propertyName, strlen(propertyName));
        strupr(*propName);

        int i = 0;
        for(; i < hubClient->desiredCallbackCount; i++) {
            if (strcmp(*propName, hubClient->desiredCallbackList[i].name) == 0) {
                status = hubClient->desiredCallbackList[i].callback(payLoad,
                            size, &methodResponse, &responseSize);
                echoDesired(propertyName, payLoad, methodResponse, status);
                break;
            }
        }

        if (i == hubClient->desiredCallbackCount) {
            LOG_ERROR("Property Name '%s' is not found @callDesiredCallback", *propName);
        }
    }
}

void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state,
    const unsigned char* payLoad, size_t size, void* userContextCallback) {
    LOG_VERBOSE("IoTHubClient::deviceTwinGetStateCallback (%s)", payLoad);

    WatchdogController::reset();
    ((char*)payLoad)[size] = 0x00;
    JSObject payloadObject((const char *)payLoad);

    if (update_state == DEVICE_TWIN_UPDATE_PARTIAL && payloadObject.getNameAt(0) != NULL) {
        callDesiredCallback(payloadObject.getNameAt(0), reinterpret_cast<const char*>(payLoad), size);
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
                    callDesiredCallback(itemName, (const char*)payLoad, size);
                } else {
                    echoDesired(itemName, (const char*)payLoad, Globals::completedString, 200);
                }
            }
        }
    }
}

static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback) {
    LOG_VERBOSE("DeviceTwin CallBack: Status_code = %u", status_code);
}

static void connectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {
    LOG_VERBOSE("IoTHubClient::connectionStatusCallback result:%d", result);

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*) Globals::loopController;
    } else {
        return;
    }

    assert(telemetryController != NULL && telemetryController->getHubClient() != NULL);

    if (reason == IOTHUB_CLIENT_CONNECTION_NO_NETWORK) {
        LOG_ERROR("No network connection");
        telemetryController->getHubClient()->needsReconnect = true;
    } else if (result == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED &&
            reason == IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN) {
        LOG_ERROR("Connection timeout");
        telemetryController->getHubClient()->needsReconnect = true;
    }
    telemetryController->getHubClient()->checkConnection();
}

static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
    // confirmation tells us that we were connected and things were working.
    // so reset the watchdog controller until the next time.
    static bool firstReset = true;
    WatchdogController::reset(firstReset /* send telemetry */);
    firstReset = false;

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*) Globals::loopController;
        telemetryController->setCanSend(true);
    }

    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;
    LOG_VERBOSE("Confirmation received for message tracking id = %d \
        with result = %s", eventInstance->messageTrackingId,
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
