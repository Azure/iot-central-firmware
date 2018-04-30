// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef CONFIG_H
#define CONFIG_H

#include "utility.h"

void clearAllConfig();

void storeWiFi(AutoString &ssid, AutoString &password);
void storeConnectionString(AutoString &connectionString);
void storeIotCentralConfig(AutoString &iotCentralConfig);

void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen);
void readConnectionString(char * connectionString, uint32_t buffer_size);
void readIotCentralConfig(char * iotCentralConfig, uint32_t buffer_size);

void clearWiFiEEPROM();
void clearAzureEEPROM();
void clearIotCentralEEPROM();
#endif /* CONFIG_H */
