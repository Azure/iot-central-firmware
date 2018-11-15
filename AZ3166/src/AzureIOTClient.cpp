// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include <time.h>

#include "../inc/telemetry.h"
#include "../inc/utility.h"
#include "../inc/config.h"

#include "../inc/stats.h"
#include "../inc/wifi.h"

#include "../inc/watchdogController.h"

void onMessageSent(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    static bool firstReset = true;
    WatchdogController::reset(firstReset /* send telemetry */);
    firstReset = false;

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*) Globals::loopController;
        telemetryController->setCanSend(true);
    }

    if (callbackInfo.appContext != NULL) {
        int *id = (int*)callbackInfo.appContext;
        LOG_VERBOSE("Confirmation received for message tracking id = %d", *id);
        delete id;
    } else {
        LOG_VERBOSE("deviceTwinConfirmationCallback was received");
    }
}

void onMessageReceived(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    LOG_VERBOSE("AzureIOTClient::receiveMessageCallback (%s)", callbackInfo.payload);

    const char *buffer = callbackInfo.payload;
    unsigned size = callbackInfo.payload_length;

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
        callbackInfo.statusCode = IOTC_MESSAGE_REJECTED;
        return;
    }
    methodName.toUpperCase();

    AzureIOTClient *client = (AzureIOTClient*) callbackInfo.appContext;

    if (client != NULL) {
        // lookup if the method has been registered to a function
        for(int i = 0; i < client->methodCallbackCount; i++) {
            if (methodName == client->methodCallbackList[i].name) {
                const char* payload = json.getStringByName("payload");
                if (payload == NULL) {
                    LOG_ERROR("Object doesn't have a member 'payload'");
                    return;
                }

                char *pcopy = strdup((const char*)payload);
                assert(pcopy);
                char *mcopy = strdup(methodName.c_str());
                assert(mcopy);

                client->pushDirectMethod(mcopy, pcopy, size);
                break;
            }
        }
    }

    return;
}

void onCommand(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    LOG_VERBOSE("AzureIOTClient::onCommand (methodName:%s)", callbackInfo.tag);
    WatchdogController::reset();

    AzureIOTClient *client = (AzureIOTClient*) callbackInfo.appContext;

    if (client != NULL) {
        // lookup if the method has been registered to a function
        String methodName = callbackInfo.tag;
        methodName.toUpperCase();
        for(int i = 0; i < client->methodCallbackCount; i++) {
            if (methodName == client->methodCallbackList[i].name) {
                AutoString payloadCopy((const char*)callbackInfo.payload, callbackInfo.payload_length); // payload may not be null ended
                payloadCopy.makePersistent();
                char *pcopy = *payloadCopy;
                assert(pcopy);
                char *mcopy = strdup(methodName.c_str());
                assert(mcopy);

                client->pushDirectMethod(mcopy, pcopy, callbackInfo.payload_length);
                break;
            }
        }
    }
}

void onConnectionStatus(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    LOG_VERBOSE("AzureIOTClient::connectionStatusCallback result:%d", callbackInfo.statusCode);
    AzureIOTClient *client = (AzureIOTClient*) callbackInfo.appContext;
    assert(client != NULL);

    if (callbackInfo.statusCode != IOTC_CONNECTION_OK) {
        if (callbackInfo.statusCode == IOTC_CONNECTION_DEVICE_DISABLED) {
            LOG_ERROR("Device was disabled.");
        } else if (callbackInfo.statusCode == IOTC_CONNECTION_NO_NETWORK) {
            LOG_ERROR("No network connection");
            client->needsReconnect = true;
        } else if (callbackInfo.statusCode != IOTC_CONNECTION_BAD_CREDENTIAL) {
            LOG_ERROR("Connection timeout");
            client->needsReconnect = true;
        } else {
            LOG_ERROR("Bad credentials");
            client->needsReconnect = true;
        }
        client->checkConnection();
    }
}

void onError(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    LOG_ERROR("onError => %s", callbackInfo.tag);
}

void onSettingsUpdated(IOTContext ctx, IOTCallbackInfo &callbackInfo) {
    AzureIOTClient *client = (AzureIOTClient*) callbackInfo.appContext;
    assert(client != NULL);
    int i = 0;
    AutoString propName(callbackInfo.tag, strlen(callbackInfo.tag));
    strupr(*propName);

    for(; i < client->desiredCallbackCount; i++) {
        if (strcmp(*propName, client->desiredCallbackList[i].name) == 0) {
            client->desiredCallbackList[i].callback(callbackInfo.payload, callbackInfo.payload_length);
            break;
        }
    }

    if (i == client->desiredCallbackCount) {
        callbackInfo.callbackResponse = strdup("TargetNotFound"); // iotc will free the memory
        LOG_ERROR("Property Name '%s' is not found @callDesiredCallback", *propName);
    }
}

void AzureIOTClient::init() {
    LOG_VERBOSE("AzureIOTClient::init START");
    char stringBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    char scopeId[STRING_BUFFER_128] = {0};
    char registrationId[STRING_BUFFER_128] = {0};
    bool sasKey = false;

    iotc_set_logging(IOTHUB_TRACE_LOG_ENABLED ? IOTC_LOGGING_ALL : (
                        SERIAL_VERBOSE_LOGGING_ENABLED ?
                            IOTC_LOGGING_API_ONLY : IOTC_LOGGING_DISABLED));

    ConfigController::readGroupSXKeyAndDeviceId(scopeId, registrationId, stringBuffer, sasKey);
    LOG_VERBOSE("SCOPEID: %s REGID: %s KEY: %s IS_SAS: %d", scopeId, registrationId, stringBuffer, sasKey);

    int errorCode = iotc_init_context(&context);
    assert(errorCode == 0);
    errorCode = iotc_connect(context, scopeId, stringBuffer, registrationId, sasKey ? IOTC_CONNECT_SYMM_KEY : IOTC_CONNECT_X509_CERT);
    assert(errorCode == 0);

    iotc_on(context, "MessageSent", onMessageSent, this);
    iotc_on(context, "MessageReceived", onMessageReceived, this);
    iotc_on(context, "Command", onCommand, this);
    iotc_on(context, "ConnectionStatus", onConnectionStatus, this);
    iotc_on(context, "SettingsUpdated", onSettingsUpdated, this);
    iotc_on(context, "Error", onError, this);

    WatchdogController::reset();
    LOG_VERBOSE("AzureIOTClient::init END");
}

bool AzureIOTClient::sendTelemetry(const char *payload) {
    checkConnection();
    static unsigned long trackingId = 0;
    trackingId++;

    int *id = new int;
    *id = trackingId;
    return iotc_send_telemetry(context, payload, strlen(payload), id) == 0;
}

bool AzureIOTClient::sendReportedProperty(const char *payload) {
    checkConnection();

    return iotc_send_property(context, payload, strlen(payload), NULL) == 0;
}

// register callbacks for direct and cloud to device messages
bool AzureIOTClient::registerMethod(const char *methodName, hubMethodCallback callback) {
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
bool AzureIOTClient::registerDesiredProperty(const char *propertyName, hubMethodCallback callback) {
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

void AzureIOTClient::close()
{
    iotc_disconnect(context);
    iotc_free_context(context);
    context = NULL;
    LOG_ERROR("AzureIOTClient::close!");
}

// scrolling text global variables
void AzureIOTClient::displayDeviceInfo() {
    // char buff[STRING_BUFFER_128] = {0};

    // if (needsCopying) {
    //     // code to scroll the larger hubname if it exceeds 16 characters
    //     if (waitCount >= 3) {
    //         waitCount = 0;
    //         const size_t hubNameLength = strlen(hubName);
    //         if (hubNameLength > AZ3166_DISPLAY_MAX_COLUMN) {
    //             unsigned length = min(hubNameLength - displayCharPos, AZ3166_DISPLAY_MAX_COLUMN);
    //             memcpy(displayHubName, hubName + displayCharPos, length);
    //             displayHubName[length] = char(0); // future proof
    //             if ((size_t) AZ3166_DISPLAY_MAX_COLUMN + displayCharPos >= hubNameLength) {
    //                 displayCharPos = 0;
    //             } else {
    //                 displayCharPos++;
    //             }
    //         } else { // hubNameLength > AZ3166_DISPLAY_MAX_COLUMN
    //             memcpy(displayHubName, hubName, hubNameLength);
    //             displayHubName[hubNameLength] = char(0); // future proof
    //             needsCopying = false; // no need to do this again
    //         } // hubNameLength > AZ3166_DISPLAY_MAX_COLUMN
    //     } else {
    //         waitCount++;
    //     } // waitCount >= 3
    // } // needsCopying

    // snprintf(buff, STRING_BUFFER_128 - 1, "Device:\r\n%s\r\n%.16s\r\nf/w: %s",
    //     deviceId, displayHubName, AZIOTC_FW_VERSION);
    // Screen.print(0, buff);
}
