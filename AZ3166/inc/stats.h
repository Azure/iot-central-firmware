// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef STATS_H
#define STATS_H

void clearCounters();

void incrementReportedCount();
void incrementErrorCount();
void incrementTelemetryCount();
void incrementDesiredCount();

int getReportedCount();
int getErrorCount();
int getTelemetryCount();
int getDesiredCount();

#endif /* STATS_H */