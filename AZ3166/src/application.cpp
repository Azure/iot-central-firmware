// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/application.h"
#include "../inc/onboarding.h"
#include "../inc/telemetry.h"
#include "../inc/config.h"
#include "../inc/device.h"

bool ApplicationController::initialize() {
    Serial.begin(250000);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_AZURE, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    char iotCentralConfig[IOT_CENTRAL_MAX_LEN] = {0};
    ConfigController::readIotCentralConfig(iotCentralConfig, IOT_CENTRAL_MAX_LEN);

    assert(Globals::loopController == NULL);
    if (iotCentralConfig[0] == 0 && iotCentralConfig[2] == 0) {
        LOG_ERROR("No configuration found...");
        Globals::loopController = OnboardingController::New();
    } else {
        LOG_VERBOSE("Configuration found entering telemetry mode.");
        Globals::loopController = TelemetryController::New(iotCentralConfig);
    }
}

bool ApplicationController::loop() {
    // reset the device if the A and B buttons are both pressed and held
    if (DeviceControl::IsButtonClicked(USER_BUTTON_A) &&
        DeviceControl::IsButtonClicked(USER_BUTTON_B)) {

        Screen.clean();
        Screen.print(0, "RE-initializing...");
        delay(1000);  //artificial pause
        Screen.clean();
        ConfigController::clearAllConfig();

        if (Globals::loopController != NULL) {
            delete Globals::loopController;
            Globals::loopController = NULL;
        }
        initialize();
    }

    if (Globals::loopController != NULL) {
        Globals::loopController->loop();
    }
}