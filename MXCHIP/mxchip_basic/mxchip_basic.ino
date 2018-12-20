// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define SERIAL_VERBOSE_LOGGING_ENABLED 1
#include "src/iotc/iotc.h"
#include <string.h>
#include <EEPROMInterface.h>
#include "AZ3166WiFi.h"

static IOTContext context = NULL;

#define WIFI_SSID "<Enter WiFi SSID here>"
#define WIFI_PASSWORD "<Enter WiFi Password here>"

// CONNECTION STRING ??
// Uncomment below to Use Connection String
// IOTConnectType connectType = IOTC_CONNECT_CONNECTION_STRING;
// const char* scopeId = ""; // leave empty
// const char* deviceId = ""; // leave empty
// const char* deviceKey = "<ENTER CONNECTION STRING HERE>";

// OR

// PRIMARY/SECONDARY KEY ?? (DPS)
// Uncomment below to Use DPS Symm Key (primary/secondary key..)
// IOTConnectType connectType = IOTC_CONNECT_SYMM_KEY;
// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";

static bool isConnected = false;

void onEvent(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
        LOG_VERBOSE("Is connected ? %s (%d)", callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO", callbackInfo->statusCode);
        isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    }
    LOG_VERBOSE("- [%s] event was received. Payload => %s", callbackInfo->eventName, callbackInfo->payload != NULL ? callbackInfo->payload : "None");
}

void setup()
{
    Serial.begin(9600);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_AZURE, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    // WiFi setup
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) WIFI_SSID, strlen(WIFI_SSID), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*) WIFI_PASSWORD, strlen(WIFI_PASSWORD), WIFI_PWD_ZONE_IDX);

    if(WiFi.begin() == WL_CONNECTED) {
        LOG_VERBOSE("WiFi WL_CONNECTED");
        digitalWrite(LED_WIFI, 1);
        Screen.print(2, "Connected");
    } else {
        Screen.print("WiFi\r\nNot Connected\r\nWIFI_SSID?\r\n");
        return;
    }

    // Azure IOT Central setup
    int errorCode = iotc_init_context(&context);
    if (errorCode != 0) {
        LOG_ERROR("Error initializing IOTC. Code %d", errorCode);
        return;
    }

    iotc_set_logging(IOTC_LOGGING_API_ONLY);

    errorCode = iotc_connect(context, scopeId, deviceKey, deviceId, connectType);
    if (errorCode != 0) {
        LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
        return;
    }

    // for the simplicity of this sample, used same callback for all the events below
    iotc_on(context, "MessageSent", onEvent, NULL);
    iotc_on(context, "MessageReceived", onEvent, NULL);
    iotc_on(context, "Command", onEvent, NULL);
    iotc_on(context, "ConnectionStatus", onEvent, NULL);
    iotc_on(context, "SettingsUpdated", onEvent, NULL);
    iotc_on(context, "Error", onEvent, NULL);
}

static unsigned counter = 0;
void loop()
{
    if (!context) return;

    if (counter++ % 5 == 0 && isConnected) { // send telemetry every 5 seconds
        char msg[64] = {0};
        int pos = snprintf(msg, sizeof(msg) - 1, "{\"temp\": %d}", 10 + (rand() % 20));
        msg[pos] = 0;
        int errorCode = iotc_send_telemetry(context, msg, pos, NULL);
        Screen.print(0, "Sent..");
        Screen.print(1, msg);

        if (errorCode != 0) {
            LOG_ERROR("Sending message has failed with error code %d", errorCode);
        }
    }

    delay(1000); // wait 1 sec
    iotc_do_work(context); // do background work for iotc
}
