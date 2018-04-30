// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

static int telemetryCount;
static int reportedCount;
static int desiredCount;
static int errorCount;

void clearCounters() {
    telemetryCount = 0;
    reportedCount = 0;
    desiredCount = 0;
    errorCount = 0;
}

void incrementReportedCount() {
    reportedCount++;
}

void incrementErrorCount() {
    if (telemetryCount < 99999999) {
        errorCount++;
    } else {
        errorCount = 0;
    }
}

void incrementTelemetryCount() {
    if (telemetryCount < 99999999) {
        telemetryCount++;
    } else {
        telemetryCount = 0;
    }
}

void incrementDesiredCount() {
    desiredCount++;
}

int getReportedCount(){
    return reportedCount;
}

int getErrorCount() {
    return errorCount;
}

int getTelemetryCount() {
    return telemetryCount;
}

int getDesiredCount() {
    return desiredCount;
}