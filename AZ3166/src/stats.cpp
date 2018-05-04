// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/stats.h"

int StatsController::telemetryCount = 0;
int StatsController::reportedCount = 0;
int StatsController::desiredCount = 0;
int StatsController::errorCount = 0;

void StatsController::clearCounters() {
    telemetryCount = 0;
    reportedCount = 0;
    desiredCount = 0;
    errorCount = 0;
}

void StatsController::incrementReportedCount() {
    reportedCount++;
}

void StatsController::incrementErrorCount() {
    if (telemetryCount < 99999999) {
        errorCount++;
    } else {
        errorCount = 0;
    }
}

void StatsController::incrementTelemetryCount() {
    if (telemetryCount < 99999999) {
        telemetryCount++;
    } else {
        telemetryCount = 0;
    }
}

void StatsController::incrementDesiredCount() {
    desiredCount++;
}

int StatsController::getReportedCount(){
    return reportedCount;
}

int StatsController::getErrorCount() {
    return errorCount;
}

int StatsController::getTelemetryCount() {
    return telemetryCount;
}

int StatsController::getDesiredCount() {
    return desiredCount;
}