// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include "../inc/telemetry.h"
#include "../inc/config.h"
#include "../inc/wifi.h"
#include "../inc/sensors.h"
#include "../inc/utility.h"
#include "../inc/AzureIOTClient.h"
#include "../inc/device.h"
#include "../inc/stats.h"
#include "../inc/registeredMethodHandlers.h"
#include "../inc/oledAnimation.h"
#include "../inc/watchdogController.h"

void TelemetryController::initializeTelemetryController(const char * iotCentralConfig) {
    assert(initializeCompleted == false);

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
    WatchdogController::reset();

    // initialize the IoT Hub Client
    assert(iotClient == NULL);
    iotClient = new AzureIOTClient();
    if (iotClient == NULL || !iotClient->wasInitialized()) {
        if (iotClient != NULL) {
            delete iotClient;
            iotClient = NULL;
        }
        Screen.print(0, "Error:");
        Screen.print(1, "");
        Screen.print(2, "Please reset \r\n   the device.  \r\n");
        return;
    }
    WatchdogController::reset();

    // Register callbacks for direct messages
    iotClient->registerCallback("echo", dmEcho);
    iotClient->registerCallback("countdown", dmCountdown);

    // register callbacks for desired properties expected
    iotClient->registerCallback("fanSpeed", fanSpeedDesiredChange);
    iotClient->registerCallback("setVoltage", voltageDesiredChange);
    iotClient->registerCallback("setCurrent", currentDesiredChange);
    iotClient->registerCallback("activateIR", irOnDesiredChange);

    // show the state of the device on the RGB LED
    DeviceControl::showState();

    // clear all the stat counters
    StatsController::clearCounters();

    assert(iotCentralConfig != NULL);
    telemetryState = strtol(iotCentralConfig, NULL, 10);

    sendStateChange();

    initializeCompleted = true;
}

static int  locationDataOffset = 0;
static char locationString[STRING_BUFFER_128];
static int  sendCount = -1;

void TelemetryController::loop() {
    // if we are about to reset then stop sending/processing any telemetry
    if (!initializeCompleted || !Globals::wiFiController.getIsConnected()) {
        if (!Globals::wiFiController.getIsConnected()) {
            Screen.print(0, "-NOT Connected- ");
            Screen.print(1, "                ");
            Screen.print(2, " Check WiFi ?   ");
            Screen.print(3, "                ");
        }

        delay(1);
        return;
    }

    const uint32_t currentMillis = millis(); // reduce number of times we call this

    if ((currentMillis - lastTimeSync > NTP_SYNC_PERIOD)) {
        // re-sync the time from ntp
        if (SyncTimeToNTP()) {
            lastTimeSync = millis();
        }
    }

    // look for button A pressed to signify state change
    // when the A button is pressed the device state rotates to the next value and a state telemetry message is sent
    if (DeviceControl::IsButtonClicked(USER_BUTTON_A) &&
        (currentMillis - lastSwitchPress > TELEMETRY_SWITCH_DEBOUNCE_TIME)) {

        DeviceControl::incrementDeviceState();
        DeviceControl::showState();
        sendStateChange();
        lastSwitchPress = millis();
    }

    // look for button B pressed to page through info screens
    if (DeviceControl::IsButtonClicked(USER_BUTTON_B) &&
        (currentMillis - lastSwitchPress > TELEMETRY_SWITCH_DEBOUNCE_TIME)) {

        currentInfoPage = (currentInfoPage + 1) % 3;
        lastSwitchPress = currentMillis;

        // SEND EVENT example
        // build the event payload
        const char * eventString = "{\"ButtonBPressed\": \"occurred\"}";
        if (iotClient->sendTelemetry(eventString)) {
            LOG_VERBOSE("Event successfully sent");
            StatsController::incrementReportedCount();
        } else {
            LOG_ERROR("Sending event has failed");
            StatsController::incrementErrorCount();
        }
    }

    // example of sending telemetry data
    if (canSend() && currentMillis - lastTelemetrySend >= TELEMETRY_SEND_INTERVAL) {
        setCanSend(false); // wait until the telemetry is sent

        // send location once on each fifth (aprox 30 secs)
        if (++sendCount % 6 == 0) {
            int n = snprintf(locationString, STRING_BUFFER_128 - 1,
                    "{\"location\":{\"lon\":%f,\"lat\":%f}}",
                    Globals::locationData[locationDataOffset++],
                    Globals::locationData[locationDataOffset++]);
            locationString[n] = 0;
            locationDataOffset %= MAP_DATA_SIZE;
            if (iotClient->sendReportedProperty(locationString)) {
                LOG_VERBOSE("Reported property location successfully sent %s", locationString);
                StatsController::incrementReportedCount();
            } else {
                LOG_ERROR("Reported property location failed to during sending");
                StatsController::incrementErrorCount();
            }
            sendCount = 0;
        }

        String payload; // max payload size for Azure IoT
        buildTelemetryPayload(&payload);
        sendTelemetryPayload(payload.c_str());
        lastTelemetrySend = millis();
    }

    DirectMethodNode * task = iotClient->popDirectMethod();
    if (task) {
        string methodNameStr = task->methodName;
        auto it = iotClient->methodCallbacks.find(methodNameStr);
        if (it != iotClient->methodCallbacks.end()) {
            it->second(task->payload, task->length);
        } else {
            LOG_ERROR("task method name wasn't registered: (%s)", task->methodName);
        }
        iotClient->freeDirectMethod(task);
    }

#ifndef DISABLE_SHAKE
    // example of sending a device twin reported property when the accelerometer detects a double tap
    if ((currentMillis - lastShakeTime > TELEMETRY_REPORTED_SEND_INTERVAL) &&
        Globals::sensorController.checkForShake()) {

        String shakeProperty = F("{\"dieNumber\":{{die}}}");
        randomSeed(analogRead(0));
        int die = random(1, 7);
        shakeProperty.replace("{{die}}", String(die));

        AnimationController::rollDieAnimation(die);

        StringBuffer shakeString(shakeProperty.c_str(), shakeProperty.length());
        if (iotClient->sendReportedProperty(*shakeString)) {
            LOG_VERBOSE("Reported property dieNumber successfully sent");
            StatsController::incrementReportedCount();
        } else {
            LOG_ERROR("Reported property dieNumber failed to during sending");
            StatsController::incrementErrorCount();
        }
        lastShakeTime = millis();
    }
#endif // DISABLE_SHAKE

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
                    StatsController::getTelemetryCount(),
                    StatsController::getErrorCount(),
                    StatsController::getDesiredCount(),
                    StatsController::getReportedCount());

            Screen.print(0, buff);
        }
            break;
        case 1: // Device information
            iotClient->displayDeviceInfo();
            break;
        case 2:  // Network information
            Globals::wiFiController.displayNetworkInfo();
            break;
    }

    iotClient->hubClientYield();
    delay(1);  // good practice to help prevent lockups
}

TelemetryController::~TelemetryController() {
    initializeCompleted = false;

    if (iotClient != NULL) {
        // cleanup the Azure IoT client
        delete iotClient;
        iotClient = NULL;
    }

    // cleanup the WiFi
    Globals::wiFiController.shutdownWiFi();
}

void TelemetryController::buildTelemetryPayload(String *payload) {
    *payload = "{";

#ifndef DISABLE_HUMIDITY
    // HTS221
    float humidity = 0.0;
    if ((telemetryState & HUMIDITY_CHECKED) == HUMIDITY_CHECKED) {
        humidity = Globals::sensorController.readHumidity();
        payload->concat(",\"humidity\":");
        payload->concat(String(humidity));
    }
#endif

#ifndef DISABLE_TEMPERATURE
    float temp = 0.0;
    if ((telemetryState & TEMP_CHECKED) == TEMP_CHECKED) {
        temp = Globals::sensorController.readTemperature();
        payload->concat(",\"temp\":");
        payload->concat(String(temp));
    }
#endif // DISABLE_TEMPERATURE

#ifndef DISABLE_PRESSURE
    // LPS22HB
    float pressure = 0.0;
    if ((telemetryState & PRESSURE_CHECKED) == PRESSURE_CHECKED) {
        pressure = Globals::sensorController.readPressure();
        payload->concat(",\"pressure\":");
        payload->concat(String(pressure));
    }
#endif // DISABLE_PRESSURE

#ifndef DISABLE_MAGNETOMETER
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
#endif // DISABLE_MAGNETOMETER

#ifndef DISABLE_ACCELEROMETER
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
#endif // DISABLE_ACCELEROMETER

#ifndef DISABLE_GYROSCOPE
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
#endif // DISABLE_GYROSCOPE

    payload->concat("}");
    payload->replace("{,", "{");
}

void TelemetryController::sendTelemetryPayload(const char *payload) {
    LOG_VERBOSE("TelemetryController::sendTelemetryPayload");
    if (iotClient->sendTelemetry(payload)) {
        // flash the Azure LED
        digitalWrite(LED_AZURE, 1);
        delay(500);
        digitalWrite(LED_AZURE, 0);
        StatsController::incrementTelemetryCount();
    } else {
        digitalWrite(LED_USER, 1);
        delay(500);
        digitalWrite(LED_USER, 0);
        StatsController::incrementErrorCount();
    }
}

void TelemetryController::sendStateChange() {
    // SEND State example
    assert (iotClient != NULL && iotClient->wasInitialized());
    const char * stateMessage = (STATE_MESSAGE(DeviceControl::getDeviceState()));
    sendTelemetryPayload(stateMessage);
    iotClient->hubClientYield();
    delay(1);  // good practice to help prevent lockups
}