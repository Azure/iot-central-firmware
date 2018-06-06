// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/watchdogController.h"

Watchdog WatchdogController::watchdog;

void WatchdogController::initialize() {
    if (!watchdog.configure(32767))
    {
        // unlikely. returns false only when the timeout is beyond the supported range
        // Min timeout period at 32kHz LSI: 0.125ms, with 4 prescaler divider
        // Max timeout period: 32768ms, with 256 prescaler divider
        LOG_ERROR("watchdog initialization has failed.");
    }
}

void WatchdogController::reset() {
    watchdog.resetTimer();
}