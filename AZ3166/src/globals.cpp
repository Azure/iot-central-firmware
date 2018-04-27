// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/config.h"
#include "../inc/webServer.h"

bool GlobalConfig::needsReconnect = false;
bool GlobalConfig::isConfigured = false;
bool GlobalConfig::needsInitialize = true;
AzWebServer GlobalConfig::webServer;
const char * GlobalConfig::completedString = "completed";
