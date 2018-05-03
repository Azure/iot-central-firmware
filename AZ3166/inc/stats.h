// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef STATS_H
#define STATS_H

// SINGLETON
class StatsController {
    static int telemetryCount;
    static int reportedCount;
    static int desiredCount;
    static int errorCount;

public:
    static void clearCounters();

    static void incrementReportedCount();
    static void incrementErrorCount();
    static void incrementTelemetryCount();
    static void incrementDesiredCount();

    static int getReportedCount();
    static int getErrorCount();
    static int getTelemetryCount();
    static int getDesiredCount();
};

#endif /* STATS_H */