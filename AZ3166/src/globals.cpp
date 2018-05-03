// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/config.h"
#include "../inc/wifi.h"
#include "../inc/sensors.h"
#include "../inc/loop.h"

const char *     Globals::completedString = "completed";
WiFiController   Globals::wiFiController;
SensorController Globals::sensorController;
LoopController * Globals::loopController = NULL;