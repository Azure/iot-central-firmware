// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "src/iotc/iotc.h"
#include "src/iotc/common/string_buffer.h"
#include <ESP8266WiFi.h>

// #define WIFI_SSID "<ENTER WIFI SSID HERE>"
// #define WIFI_PASSWORD "<ENTER WIFI PASSWORD HERE>"

// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";

static IOTContext context = NULL;

void connect_wifi() {
    Serial.begin(9600);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    LOG_VERBOSE("Connecting WiFi..");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

static bool isConnected = false;

void onEvent(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
        LOG_VERBOSE("Is connected ? %s (%d)", callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO", callbackInfo->statusCode);
        isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    }

    AzureIOT::StringBuffer buffer;
    if (callbackInfo->payloadLength > 0) {
        buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
    }
    LOG_VERBOSE("- [%s] event was received. Payload => %s", callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

    if (strcmp(callbackInfo->eventName, "Command") == 0) {
        LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
    }
}

static unsigned prevMillis = 0, loopId = 0;
void setup()
{
    connect_wifi();

    // Azure IOT Central setup
    int errorCode = iotc_init_context(&context);
    if (errorCode != 0) {
        LOG_ERROR("Error initializing IOTC. Code %d", errorCode);
        return;
    }

    iotc_set_logging(IOTC_LOGGING_API_ONLY);

    // for the simplicity of this sample, used same callback for all the events below
    iotc_on(context, "MessageSent", onEvent, NULL);
    iotc_on(context, "Command", onEvent, NULL);
    iotc_on(context, "ConnectionStatus", onEvent, NULL);
    iotc_on(context, "SettingsUpdated", onEvent, NULL);
    iotc_on(context, "Error", onEvent, NULL);

    errorCode = iotc_connect(context, scopeId, deviceKey, deviceId, IOTC_CONNECT_SYMM_KEY);
    if (errorCode != 0) {
        LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
        return;
    }
    prevMillis = millis();
}

void loop()
{
    if (isConnected) {
        unsigned long ms = millis();
        if (ms - prevMillis > 15000) { // send telemetry every 15 seconds
            char msg[64] = {0};
            int pos = 0, errorCode = 0;

            prevMillis = ms;
            if (loopId++ % 2 == 0) {
                pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerX\": %d}", 10 + (rand() % 20));
                errorCode = iotc_send_telemetry(context, msg, pos);
            } else {
                pos = snprintf(msg, sizeof(msg) - 1, "{\"dieNumber\":%d}", 1 + (rand() % 5));
                errorCode = iotc_send_property(context, msg, pos);
            }
            msg[pos] = 0;

            if (errorCode != 0) {
                LOG_ERROR("Sending message has failed with error code %d", errorCode);
            }
        }

        iotc_do_work(context); // do background work for iotc
    }
}

