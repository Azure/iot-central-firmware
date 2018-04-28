// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/config.h"
#include "../inc/webServer.h"

bool Globals::isConfigured = false;
bool Globals::needsInitialize = true;
AzWebServer Globals::webServer;
const char * Globals::completedString = "completed";
IoTHubClient * Globals::iothubClient = NULL;