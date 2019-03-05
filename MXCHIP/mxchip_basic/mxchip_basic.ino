// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define SERIAL_VERBOSE_LOGGING_ENABLED 1
#include "src/iotc/iotc.h"
#include "src/iotc/common/string_buffer.h"
#include "LSM6DSLSensor.h"
#include "LIS2MDLSensor.h"
#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "EEPROMInterface.h"
#include "IoT_DevKit_HW.h"
#include "AZ3166WiFi.h"

static IOTContext context = NULL;

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

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
const char *scopeId = "";
const char *deviceId = "";
const char *deviceKey = "=";

static bool isConnected = false;

void onEvent(IOTContext ctx, IOTCallbackInfo *callbackInfo)
{
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

static DevI2C *i2c;
static LSM6DSLSensor *accelerometerGyroscopeSensor;
static HTS221Sensor *humidityTemperatureSensor;
static LIS2MDLSensor *magnetometerSensor;
static LPS22HBSensor *pressureSensor;

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

        i2c = new DevI2C(D14, D15);

        accelerometerGyroscopeSensor = new LSM6DSLSensor(*i2c, D4, D5);
        accelerometerGyroscopeSensor->init(NULL);
        accelerometerGyroscopeSensor->enableAccelerator();
        accelerometerGyroscopeSensor->enableGyroscope();

        magnetometerSensor = new LIS2MDLSensor(*i2c);
        magnetometerSensor->init(NULL);

        humidityTemperatureSensor = new HTS221Sensor(*i2c);
        humidityTemperatureSensor->init(NULL);

        pressureSensor = new LPS22HBSensor(*i2c);
        pressureSensor->init(NULL);

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

    // for the simplicity of this sample, used same callback for all the events below
    iotc_on(context, "MessageSent", onEvent, NULL);
    iotc_on(context, "Command", onEvent, NULL);
    iotc_on(context, "ConnectionStatus", onEvent, NULL);
    iotc_on(context, "SettingsUpdated", onEvent, NULL);
    iotc_on(context, "Error", onEvent, NULL);

    errorCode = iotc_connect(context, scopeId, deviceKey, deviceId, connectType);
    if (errorCode != 0) {
        LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
        return;
    }
    LOG_VERBOSE("Done!");

    prevMillis = millis();
}

void loop()
{
    if (isConnected) {
        unsigned long ms = millis();
        if (ms - prevMillis > 15000) { // send telemetry every 15 seconds
            char msg[64] = {0};

            int pos = 0, errorCode = 0;

            int accelerometerAxes[3]; // accelerometerX accelerometerY and accelerometerZ
            int gyroscopAxes[3]; // gyroscopX gyroscopY and gyroscopZ
            float magnetometeX = 0;
            float magnetometeY = 0;
            float magnetometerZ = 0;
            float temperature = 0;
            float humidity = 0;
            float pressure = 0;

            accelerometerGyroscopeSensor->getXAxes(accelerometerAxes);
            humidityTemperatureSensor->getTemperature(&temperature);
            humidityTemperatureSensor->getHumidity(&humidity);
            pressureSensor->getPressure(&pressure);

            prevMillis = ms;
            if (loopId++ % 2 == 0) { // send telemetry
                pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerX\":%d}", accelerometerAxes[0]);
                errorCode = iotc_send_telemetry(context, msg, pos);
                pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerY\":%d}", accelerometerAxes[1]);
                errorCode = iotc_send_telemetry(context, msg, pos);
                pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerZ\":%d}", accelerometerAxes[2]);
                errorCode = iotc_send_telemetry(context, msg, pos);
                pos = snprintf(msg, sizeof(msg) - 1, "{\"temperature\":%d}", temperature);
                errorCode = iotc_send_telemetry(context, msg, pos);
                pos = snprintf(msg, sizeof(msg) - 1, "{\"humidity\":%d}", humidity);
                errorCode = iotc_send_telemetry(context, msg, pos);
                pos = snprintf(msg, sizeof(msg) - 1, "{\"pressure\":%d}", pressure);
                errorCode = iotc_send_telemetry(context, msg, pos);
            } else { // send property
                pos = snprintf(msg, sizeof(msg) - 1, "{\"dieNumber\":%d}", 1 + (rand() % 5));
                errorCode = iotc_send_property(context, msg, pos);
            }
            // msg[pos] = 0;

            if (errorCode != 0) {
                 LOG_ERROR("Sending message has failed with error code %d", errorCode);
            }

            humidityTemperatureSensor->reset();
        }
    }

    if (context) {
        iotc_do_work(context); // do background work for iotc
    }

}
