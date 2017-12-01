// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"
#include <ArduinoJson.h>

#include "../inc/main_telemetry.h"
#include "../inc/config.h"
#include "../inc/wifi.h"
#include "../inc/sensors.h"
#include "../inc/utility.h"
#include "../inc/iotHubClient.h"
#include "../inc/device.h"
#include "../inc/stats.h"
#include "../inc/registeredMethodHandlers.h"
#include "../inc/oledAnimation.h"

#define traceOn false
#define statePayloadTemplate "{\"%s\":\"%s\"}"

// forward declarations
void showState();
void sendTelemetryPayload(const char *payload);
void showState();
void sendStateChange();
void buildTelemetryPayload(String *payload);
void rollDieAnimation(int value);

const int telemetrySendInterval = 5000;
const int reportedSendInterval = 2000;

static bool reset = false;
const int switchDebounceTime = 250;
static bool connected;
unsigned long lastTimeSync = 0;
unsigned long timeSyncPeriod = 7200000;
unsigned long lastTelemetrySend = 0;
unsigned long lastShakeTime = 0;
unsigned long lastSwitchPress = 0;
static int currentInfoPage = 0;
static int lastInfoPage = -1;
uint8_t telemetryState = 0xFF;


void telemetrySetup(String iotCentralConfig) {
    reset = false;

    randomSeed(analogRead(0));

    // connect to the WiFi in config
    connected = initWiFi();
    lastTimeSync = millis();

    // initialize the sensor array
    initSensors();

    // initialize the IoT Hub Client
    initIotHubClient(traceOn);

    // Register callbacks for cloud to device messages
    registerMethod("message", cloudMessage);  // C2D message
    registerMethod("rainbow", directMethod);  // direct method

    // register callbacks for desired properties expected
    registerDesiredProperty("fanSpeed", fanSpeedDesiredChange);
    registerDesiredProperty("setVoltage", voltageDesiredChange);
    registerDesiredProperty("setCurrent", currentDesiredChange);
    registerDesiredProperty("activateIR", irOnDesiredChange);

    // show the state of the device on the RGB LED
    showState();

    // clear all the stat counters
    clearCounters();

    telemetryState = iotCentralConfig[2];
}


void telemetryLoop() {

    // if we are about to reset then stop sending/processing any telemetry
    if (reset) {
       delay(1);
      return;
    }

    if (connected && (millis() - lastTimeSync > timeSyncPeriod)) {
        // re-sync the time from ntp
        if (SyncTimeToNTP()) {
            lastTimeSync = millis();
        }
    }

    // look for button A pressed to signify state change
    // when the A button is pressed the device state rotates to the next value and a state telemetry message is sent
    if (IsButtonClicked(USER_BUTTON_A) && (millis() - lastSwitchPress > switchDebounceTime)) {
        incrementDeviceState();
        showState();
        sendStateChange();
        lastSwitchPress = millis();
    }
   
    // look for button B pressed to page through info screens
    if (IsButtonClicked(USER_BUTTON_B) && (millis() - lastSwitchPress > switchDebounceTime)) {
        currentInfoPage = (currentInfoPage + 1) % 3;
        lastSwitchPress = millis();
    }

    // example of sending telemetry data
    if (millis() - lastTelemetrySend >= telemetrySendInterval) {
        String payload; // max payload size for Azure IoT

        buildTelemetryPayload(&payload);

        sendTelemetryPayload(payload.c_str());

        lastTelemetrySend = millis();
    }

    // example of sending a device twin reported property when the accelerometer detects a double tap
    if (checkForShake() && (millis() - lastShakeTime > reportedSendInterval)) {
        String shakeProperty = F("{\"dieNumber\":{{die}}}");
        randomSeed(analogRead(0));
        int die = random(1, 7);
        shakeProperty.replace("{{die}}", String(die));
    
        Screen.clean();
        rollDieAnimation(die);

        if (sendReportedProperty(shakeProperty.c_str())) {
            Serial.println("Reported property dieNumber successfully sent");
            incrementReportedCount();
        } else {
            Serial.println("Reported property dieNumber failed to during sending");
            incrementErrorCount();
        }
        lastShakeTime = millis();
    }

    // update the current display page
    if (currentInfoPage != lastInfoPage) {
        Screen.clean();
        lastInfoPage = currentInfoPage;
    }
    switch (currentInfoPage) {
        case 0: // message counts - page 1
            char buff[64];
            sprintf(buff, "%s\r\nsent: %d\r\nfail: %d\r\ntwin: %d/%d", connected?"-- Connected --":"- disconnected -", getTelemetryCount(), getErrorCount(), getDesiredCount(), getReportedCount());
            Screen.print(0, buff);
            break;
        case 1: // Device information
            displayDeviceInfo();
            break;
        case 2:  // Network information    
            displayNetworkInfo();
            break;
    }
    
    delay(1);  // good practice to help prevent lockups
}

void telemetryCleanup() {
    reset = true;

    // cleanup the Azure IoT client
    closeIotHubClient();

    // cleanup the WiFi
    shutdownWiFi();
}

void buildTelemetryPayload(String *payload) {
    *payload = "{";

    // HTS221
    float humidity = 0.0;
    if ((telemetryState & HUMIDITY_CHECKED) == HUMIDITY_CHECKED) {
        humidity = readHumidity();
        payload->concat(",\"humidity\":");
        payload->concat(String(humidity));
    }

    float temp = 0.0;
    if ((telemetryState & TEMP_CHECKED) == TEMP_CHECKED) {
        temp = readTemperature();
        payload->concat(",\"temp\":");
        payload->concat(String(temp));
    }

    // LPS22HB
    float pressure = 0.0;
    if ((telemetryState & PRESSURE_CHECKED) == PRESSURE_CHECKED) {
        pressure = readPressure();
        payload->concat(",\"pressure\":");
        payload->concat(String(pressure));
    }

    // LIS2MDL
    int magAxes[3];
    if ((telemetryState & MAG_CHECKED) == MAG_CHECKED) {
        readMagnetometer(magAxes);
        payload->concat(",\"magnetometerX\":");
        payload->concat(String(magAxes[0]));
        payload->concat(",\"magnetometerY\":");
        payload->concat(String(magAxes[1]));
        payload->concat(",\"magnetometerZ\":");
        payload->concat(String(magAxes[2]));
    }

    // LSM6DSL
    int accelAxes[3];
    if ((telemetryState & ACCEL_CHECKED) == ACCEL_CHECKED) {
        readAccelerometer(accelAxes);
        payload->concat(",\"accelerometerX\":");
        payload->concat(String(accelAxes[0]));
        payload->concat(",\"accelerometerY\":");
        payload->concat(String(accelAxes[1]));
        payload->concat(",\"accelerometerZ\":");
        payload->concat(String(accelAxes[2]));
    }

    int gyroAxes[3];
    if ((telemetryState & GYRO_CHECKED) == GYRO_CHECKED) {
        readGyroscope(gyroAxes);
        payload->concat(",\"gyroscopeX\":");
        payload->concat(String(gyroAxes[0]));
        payload->concat(",\"gyroscopeY\":");
        payload->concat(String(gyroAxes[1]));
        payload->concat(",\"gyroscopeZ\":");
        payload->concat(String(gyroAxes[2]));
    }

    payload->concat("}");
    payload->replace("{,", "{");
}

void sendTelemetryPayload(const char *payload) {
    // Serial.println(payload);

    if (sendTelemetry(payload)) {
        // flash the Azure LED
        digitalWrite(LED_AZURE, 1);
        delay(500);
        digitalWrite(LED_AZURE, 0);
        incrementTelemetryCount();
    } else {
        digitalWrite(LED_USER, 1);
        delay(500);
        digitalWrite(LED_USER, 0);
        incrementErrorCount();
    }
}

void sendStateChange() {
    char stateChangePayload[4096];
    char value[10];

    switch(getDeviceState()) {
        case NORMAL:
            strcpy(value, "NORMAL");
            break;
        case CAUTION:
            strcpy(value, "CAUTION");
            break;
        case DANGER:
            strcpy(value, "DANGER");
            break;
        default:
            strcpy(value, "UNKNOWN");
    }

    sprintf(stateChangePayload, statePayloadTemplate, "deviceState", value);
    sendTelemetryPayload(stateChangePayload);
}

void rollDieAnimation(int value) {

    char die1[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};
    
    char die2[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '.', '.', '>',
        '<', '#', '$', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '!', '@', '>',
        '<', '.', '.', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die3[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '.', '.', '>',
        '<', '#', '$', '.', '.', '.', '.', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '.', '.', '.', '.', '!', '@', '>',
        '<', '.', '.', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die4[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die5[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die6[] = { 
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char *die[] = { die1, die2, die3, die4, die5, die6 };
    char *roll[5];

    for (int i = 0; i < 4; i++) {
        int dieRoll = random(0, 6);
        roll[i] = die[dieRoll];
    }
    roll[4] = die[value - 1];

    animationInit(roll, 5, 64, 0, 1000, true);

    // show the animation
    for(int i = 0; i < 4; i++) {
        renderNextFrame();
    }

    animationEnd();
}