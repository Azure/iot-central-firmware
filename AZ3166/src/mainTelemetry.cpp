// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include "../inc/mainTelemetry.h"
#include "../inc/config.h"
#include "../inc/wifi.h"
#include "../inc/sensors.h"
#include "../inc/utility.h"
#include "../inc/iotHubClient.h"
#include "../inc/device.h"
#include "../inc/stats.h"
#include "../inc/registeredMethodHandlers.h"
#include "../inc/oledAnimation.h"

#define statePayloadTemplate "{\"%s\":\"%s\"}"

// forward declarations
void sendTelemetryPayload(const char *payload);
void sendStateChange();
void buildTelemetryPayload(String *payload);
void rollDieAnimation(int value);

const int telemetrySendInterval = 5000;
const int reportedSendInterval = 2000;

static bool reset = false;
const int switchDebounceTime = 250;
unsigned long lastTimeSync = 0;
unsigned long timeSyncPeriod = 24 * 60 * 60 * 1000; // 24 Hours
unsigned long lastTelemetrySend = 0;
unsigned long lastShakeTime = 0;
unsigned long lastSwitchPress = 0;
static int currentInfoPage = 0;
static int lastInfoPage = -1;
uint8_t telemetryState = 0x0;


void telemetrySetup(const char* iotCentralConfig) {
    reset = false;

    randomSeed(analogRead(0));

    // connect to the WiFi in config
    Globals::wiFiController.initWiFi();
    lastTimeSync = millis();
    // initialize the sensor array
    Globals::sensorController.initSensors();
    if (!Globals::wiFiController.getIsConnected()) {
        LOG_ERROR("WiFi - NOT CONNECTED - return");
        return;
    }

    // initialize the IoT Hub Client
    assert(Globals::iothubClient == NULL);
    Globals::iothubClient = new IoTHubClient();
    if (Globals::iothubClient == NULL || !Globals::iothubClient->wasInitialized()) {
        reset = true;
        Globals::isConfigured = false;
        if (Globals::iothubClient != NULL) {
            delete Globals::iothubClient;
        }
        Globals::iothubClient = NULL;
        Screen.print(0, "Error:");
        Screen.print(1, "");
        Screen.print(2, "Please reset \r\n   the device.  \r\n");
        return;
    }

    // Register callbacks for cloud to device messages
    Globals::iothubClient->registerMethod("message", cloudMessage);  // C2D message
    Globals::iothubClient->registerMethod("rainbow", directMethod);  // direct method

    // register callbacks for desired properties expected
    Globals::iothubClient->registerDesiredProperty("fanSpeed", fanSpeedDesiredChange);
    Globals::iothubClient->registerDesiredProperty("setVoltage", voltageDesiredChange);
    Globals::iothubClient->registerDesiredProperty("setCurrent", currentDesiredChange);
    Globals::iothubClient->registerDesiredProperty("activateIR", irOnDesiredChange);

    // show the state of the device on the RGB LED
    DeviceControl::showState();

    // clear all the stat counters
    clearCounters();

    assert(iotCentralConfig != NULL);
    telemetryState = strtol(iotCentralConfig, NULL, 10);
}


void telemetryLoop() {
    // if we are about to reset then stop sending/processing any telemetry
    if (reset || !Globals::wiFiController.getIsConnected()) {
        delay(1);
        return;
    }

    if ((millis() - lastTimeSync > timeSyncPeriod)) {
        // re-sync the time from ntp
        if (SyncTimeToNTP()) {
            lastTimeSync = millis();
        }
    }

    // look for button A pressed to signify state change
    // when the A button is pressed the device state rotates to the next value and a state telemetry message is sent
    if (DeviceControl::IsButtonClicked(USER_BUTTON_A) &&
        (millis() - lastSwitchPress > switchDebounceTime)) {

        DeviceControl::incrementDeviceState();
        DeviceControl::showState();
        sendStateChange();
        lastSwitchPress = millis();
    }

    // look for button B pressed to page through info screens
    if (DeviceControl::IsButtonClicked(USER_BUTTON_B) &&
        (millis() - lastSwitchPress > switchDebounceTime)) {

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
    if (Globals::sensorController.checkForShake() &&
        (millis() - lastShakeTime > reportedSendInterval)) {

        String shakeProperty = F("{\"dieNumber\":{{die}}}");
        randomSeed(analogRead(0));
        int die = random(1, 7);
        shakeProperty.replace("{{die}}", String(die));

        rollDieAnimation(die);

        if (Globals::iothubClient->sendReportedProperty(shakeProperty.c_str())) {
            LOG_VERBOSE("Reported property dieNumber successfully sent");
            incrementReportedCount();
        } else {
            LOG_ERROR("Reported property dieNumber failed to during sending");
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
        {
            char buff[STRING_BUFFER_128] = {0};
            snprintf(buff, STRING_BUFFER_128 - 1,
                    "%s\r\nsent: %d\r\nfail: %d\r\ntwin: %d/%d",
                    "-- Connected --",
                    getTelemetryCount(), getErrorCount(), getDesiredCount(),
                    getReportedCount());

            Screen.print(0, buff);
        }
            break;
        case 1: // Device information
            Globals::iothubClient->displayDeviceInfo();
            break;
        case 2:  // Network information
            Globals::wiFiController.displayNetworkInfo();
            break;
    }

    delay(1);  // good practice to help prevent lockups
}

void telemetryCleanup() {
    reset = true;

    // cleanup the Azure IoT client
    delete Globals::iothubClient;

    // cleanup the WiFi
    Globals::wiFiController.shutdownWiFi();
}

void buildTelemetryPayload(String *payload) {
    *payload = "{";

    // HTS221
    float humidity = 0.0;
    if ((telemetryState & HUMIDITY_CHECKED) == HUMIDITY_CHECKED) {
        humidity = Globals::sensorController.readHumidity();
        payload->concat(",\"humidity\":");
        payload->concat(String(humidity));
    }

    float temp = 0.0;
    if ((telemetryState & TEMP_CHECKED) == TEMP_CHECKED) {
        temp = Globals::sensorController.readTemperature();
        payload->concat(",\"temp\":");
        payload->concat(String(temp));
    }

    // LPS22HB
    float pressure = 0.0;
    if ((telemetryState & PRESSURE_CHECKED) == PRESSURE_CHECKED) {
        pressure = Globals::sensorController.readPressure();
        payload->concat(",\"pressure\":");
        payload->concat(String(pressure));
    }

    // LIS2MDL
    int magAxes[3];
    if ((telemetryState & MAG_CHECKED) == MAG_CHECKED) {
        Globals::sensorController.readMagnetometer(magAxes);
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
        Globals::sensorController.readAccelerometer(accelAxes);
        payload->concat(",\"accelerometerX\":");
        payload->concat(String(accelAxes[0]));
        payload->concat(",\"accelerometerY\":");
        payload->concat(String(accelAxes[1]));
        payload->concat(",\"accelerometerZ\":");
        payload->concat(String(accelAxes[2]));
    }

    int gyroAxes[3];
    if ((telemetryState & GYRO_CHECKED) == GYRO_CHECKED) {
        Globals::sensorController.readGyroscope(gyroAxes);
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
    if (Globals::iothubClient->sendTelemetry(payload)) {
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
    char stateChangePayload[STRING_BUFFER_4096] = {0};
    char value[STRING_BUFFER_16] = {0};

    switch(DeviceControl::getDeviceState()) {
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

    unsigned length = snprintf(stateChangePayload, STRING_BUFFER_4096 - 1,
                              statePayloadTemplate, "deviceState", value);
    stateChangePayload[length] = char(0);

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
    const int numberOfRolls = 3;
    char *roll[numberOfRolls + 1];
    char uniqueRolls[6] = {0};
    uniqueRolls[value - 1] = 1;

    for (int i = 0; i < numberOfRolls; i++) {
re_roll:
        int dieRoll = random(0, 6);
        if (uniqueRolls[dieRoll] != 0) goto re_roll;
        uniqueRolls[dieRoll] = 1;

        roll[i] = die[dieRoll];
    }
    roll[numberOfRolls] = die[value - 1];

    // detach stack alloc
    {
        unsigned char buffer[(numberOfRolls + 1) * OLED_SINGLE_FRAME_BUFFER] = {0};
        Screen.clean();
        for(int i = 0; i < numberOfRolls + 1; i++) {
            AnimationController::renderFrameToBuffer(buffer + (i * OLED_SINGLE_FRAME_BUFFER), roll[i]);
        }

        // show the animation
        AnimationController::renderFrameToScreen(buffer, numberOfRolls + 1, true, 100);
    }

    delay(1200); // to keep last number on the screen longer
}