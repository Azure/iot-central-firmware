// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/watchdogController.h"
#include "../inc/telemetry.h"
#include "../inc/device.h"

Watchdog WatchdogController::watchdog;

void WatchdogController::initialize(int timer) {
    if (!watchdog.configure(timer))
    {
        // unlikely. returns false only when the timeout is beyond the supported range
        // Min timeout period at 32kHz LSI: 0.125ms, with 4 prescaler divider
        // Max timeout period: 32768ms, with 256 prescaler divider
        LOG_ERROR("watchdog initialization has failed.");
    }
}

void WatchdogController::reset(bool setState) {
    if (Globals::loopController && setState) {
        if (DeviceControl::getDeviceState() != NORMAL) {
            DeviceControl::setState(NORMAL);
            ((TelemetryController*)Globals::loopController)->sendStateChange();
        }
    }
    watchdog.resetTimer();
}