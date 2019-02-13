// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iotc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"

static IOTContext context = NULL;

// CONNECTION STRING ??
// Uncomment below to Use Connection String
// IOTConnectType connectType = IOTC_CONNECT_CONNECTION_STRING;
// const char* scopeId = ""; // leave empty
// const char* deviceId = ""; // leave empty
// const char* deviceKey = "<ENTER CONNECTION STRING HERE>";

// OR

// PRIMARY/SECONDARY KEY ?? (DPS)
// Uncomment below to Use DPS Symm Key (primary/secondary key..)
IOTConnectType connectType = IOTC_CONNECT_SYMM_KEY;
const char* scopeId = CONFIG_DEVICE_CREDENTIALS_SCOPEID;
const char* deviceId = CONFIG_DEVICE_CREDENTIALS_DEVICEID;
const char* deviceKey = CONFIG_DEVICE_CREDENTIALS_KEY;
static bool isConnected = false;

void onEvent(IOTContext ctx, IOTCallbackInfo *callbackInfo) {
    if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
        LOG_VERBOSE("Is connected ? %s (%d)", callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO", callbackInfo->statusCode);
        isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    }
    LOG_VERBOSE("- [%s] event was received. Payload => %s", callbackInfo->eventName, callbackInfo->payload != NULL ? callbackInfo->payload : "None");
}

static unsigned counter = 0;
void loop()
{
    while (context) {
        if (counter++ % 5 == 0 && isConnected) { // send telemetry every 5 seconds
            char msg[64] = {0};
            int pos = snprintf(msg, sizeof(msg) - 1, "{\"temp\": %d}", 10 + (esp_random() % 20));
            msg[pos] = 0;
            int errorCode = iotc_send_telemetry(context, msg, pos);

            if (errorCode != 0) {
               LOG_ERROR("Sending message has failed with error code %d", errorCode);
            } else {
                LOG_VERBOSE("SENT => %s", msg);
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        iotc_do_work(context); // do background work for iotc
    }
}

int iotc_main()
{
    // Azure IOT Central setup
    int errorCode = iotc_init_context(&context);
    if (errorCode != 0) {
        LOG_ERROR("Error initializing IOTC. Code %d", errorCode);
        return 1;
    }

    iotc_set_logging(IOTC_LOGGING_API_ONLY);

    errorCode = iotc_connect(context, scopeId, deviceKey, deviceId, connectType);
    if (errorCode != 0) {
        LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
        return 1;
    }

    // for the simplicity of this sample, used same callback for all the events below
    iotc_on(context, "MessageSent", onEvent, NULL);
    iotc_on(context, "MessageReceived", onEvent, NULL);
    iotc_on(context, "Command", onEvent, NULL);
    iotc_on(context, "ConnectionStatus", onEvent, NULL);
    iotc_on(context, "SettingsUpdated", onEvent, NULL);
    iotc_on(context, "Error", onEvent, NULL);

    loop();

    return 0;
}
