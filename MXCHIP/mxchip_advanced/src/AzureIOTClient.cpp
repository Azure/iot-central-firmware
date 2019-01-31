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

void onMessageSent(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    static bool firstReset = true;
    WatchdogController::reset(firstReset /* send telemetry */);
    firstReset = false;

    TelemetryController * telemetryController = NULL;
    if (Globals::loopController->withTelemetry()) {
        telemetryController = (TelemetryController*) Globals::loopController;
        telemetryController->setCanSend(true);
    }

    if (callbackInfo->appContext != NULL) {
        LOG_VERBOSE("Confirmation received.");
    } else {
        LOG_VERBOSE("deviceTwinConfirmationCallback was received");
    }
}

void onCommand(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    LOG_VERBOSE("AzureIOTClient::onCommand (methodName:%s)", callbackInfo->tag != NULL ? callbackInfo->tag : "None");
    WatchdogController::reset();

    AzureIOTClient *client = (AzureIOTClient*) callbackInfo->appContext;

    if (client != NULL) {
        // lookup if the method has been registered to a function
        string methodName = callbackInfo->tag;
        for (auto & ch: methodName) ch = toupper(ch);

        auto it = client->methodCallbacks.find(methodName);
        if (it != client->methodCallbacks.end()) {
            char *pcopy = (char*) malloc(callbackInfo->payloadLength + 1);
            assert(pcopy);
            memcpy(pcopy, (const char*)callbackInfo->payload, callbackInfo->payloadLength);
            pcopy[callbackInfo->payloadLength] = 0;
            char *mcopy = strdup(methodName.c_str());
            assert(mcopy);

            client->pushDirectMethod(mcopy, pcopy, callbackInfo->payloadLength);
        }
    }
}

void onConnectionStatus(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    LOG_VERBOSE("AzureIOTClient::connectionStatusCallback result:%d", callbackInfo->statusCode);
    AzureIOTClient *client = (AzureIOTClient*) callbackInfo->appContext;
    assert(client != NULL);

    if (callbackInfo->statusCode != IOTC_CONNECTION_OK) {
        if (callbackInfo->statusCode == IOTC_CONNECTION_DEVICE_DISABLED) {
            LOG_ERROR("Device was disabled.");
        } else if (callbackInfo->statusCode == IOTC_CONNECTION_NO_NETWORK) {
            LOG_ERROR("No network connection");
            client->needsReconnect = true;
        } else if (callbackInfo->statusCode != IOTC_CONNECTION_BAD_CREDENTIAL) {
            LOG_ERROR("Connection timeout");
            client->needsReconnect = true;
        } else {
            LOG_ERROR("Bad credentials");
            client->needsReconnect = true;
        }
        client->checkConnection();
    }
}

void onError(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    LOG_ERROR("onError => %s", callbackInfo->tag != NULL ? callbackInfo->tag : "None");
}

void onSettingsUpdated(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    AzureIOTClient *client = (AzureIOTClient*) callbackInfo->appContext;
    assert(client != NULL);
    int i = 0;
    StringBuffer propName(callbackInfo->tag, strlen(callbackInfo->tag));
    string propNameStr = *propName;
    for (auto & ch: propNameStr) ch = toupper(ch);

    auto it = client->methodCallbacks.find(propNameStr);
    if (it != client->methodCallbacks.end()) {
        it->second(callbackInfo->payload, callbackInfo->payloadLength);
    } else {
        callbackInfo->callbackResponse = strdup("TargetNotFound"); // iotc will free the memory
        LOG_ERROR("Property Name '%s' is not found @callDesiredCallback", *propName);
    }
}

void AzureIOTClient::init() {
    LOG_VERBOSE("AzureIOTClient::init START");
    char stringBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    char scopeId[STRING_BUFFER_128] = {0};
    char registrationId[STRING_BUFFER_128] = {0};
    char atype;

    iotc_set_logging(IOTHUB_TRACE_LOG_ENABLED ? IOTC_LOGGING_ALL : (
                        SERIAL_VERBOSE_LOGGING_ENABLED ?
                            IOTC_LOGGING_API_ONLY : IOTC_LOGGING_DISABLED));

    ConfigController::readGroupSXKeyAndDeviceId(scopeId, registrationId, stringBuffer, atype);
    LOG_VERBOSE("SCOPEID: %s REGID: %s KEY: %s AuthType: %c", scopeId, registrationId, stringBuffer, atype);

    deviceId = strdup(registrationId);
    int errorCode = iotc_init_context(&context);
    assert(errorCode == 0);

    IOTConnectType connectType;

#ifdef IOT_CENTRAL_CONNECTION_STRING
    connectType = IOTC_CONNECT_CONNECTION_STRING;
    memcpy(stringBuffer, IOT_CENTRAL_CONNECTION_STRING, strlen(IOT_CENTRAL_CONNECTION_STRING));
#else
    switch(atype) {
        case 'C':
            connectType = IOTC_CONNECT_CONNECTION_STRING;
            LOG_VERBOSE("- auth type => IOTC_CONNECT_CONNECTION_STRING");
            break;
        case 'X':
            connectType = IOTC_CONNECT_X509_CERT;
            LOG_VERBOSE("- auth type => IOTC_CONNECT_X509_CERT");
            break;
        case 'S':
            connectType = IOTC_CONNECT_SYMM_KEY;
            LOG_VERBOSE("- auth type => IOTC_CONNECT_SYMM_KEY");
            break;
        default:
            abort(); // unlikely
    }
#endif
    errorCode = iotc_connect(context, scopeId, stringBuffer, registrationId, connectType);
    assert(errorCode == 0);

    iotc_on(context, "MessageSent", onMessageSent, this);
    iotc_on(context, "Command", onCommand, this);
    iotc_on(context, "ConnectionStatus", onConnectionStatus, this);
    iotc_on(context, "SettingsUpdated", onSettingsUpdated, this);
    iotc_on(context, "Error", onError, this);

    WatchdogController::reset();
    LOG_VERBOSE("AzureIOTClient::init END");
}

bool AzureIOTClient::sendTelemetry(const char *payload) {
    checkConnection();

    return iotc_send_telemetry(context, payload, strlen(payload)) == 0;
}

bool AzureIOTClient::sendReportedProperty(const char *payload) {
    checkConnection();

    return iotc_send_property(context, payload, strlen(payload)) == 0;
}

// register callbacks for direct and cloud to device messages
bool AzureIOTClient::registerCallback(const char *methodName, hubMethodCallback callback) {
    string strName = methodName;
    for (auto & ch: strName) ch = toupper(ch);
    methodCallbacks[strName] = callback;
    return true;
}

void AzureIOTClient::close()
{
    iotc_disconnect(context);
    iotc_free_context(context);
    context = NULL;
    LOG_ERROR("AzureIOTClient::close!");
}

void AzureIOTClient::displayDeviceInfo() {
    char buff[STRING_BUFFER_128] = {0};

    snprintf(buff, STRING_BUFFER_128 - 1, "Device:\r\n%s\r\n\r\nf/w: %s",
        deviceId, AZIOTC_FW_VERSION);
    Screen.print(0, buff);
}
