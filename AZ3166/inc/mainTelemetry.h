// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef MAIN_TELEMETRY_H
#define MAIN_TELEMETRY_H

void telemetrySetup(const char * iotCentralConfig);
void telemetryLoop();
void telemetryCleanup();

#endif /* MAIN_TELEMETRY_H */