// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef CONFIG_H
#define CONFIG_H

#include "utility.h"

// SINGLETON
class ConfigController {
    ConfigController() { }
public:
    static void clearAllConfig();

    static void storeWiFi(AutoString &ssid, AutoString &password);
    static void storeConnectionString(AutoString &connectionString);
    static void storeIotCentralConfig(AutoString &iotCentralConfig);
    static void storeKey(AutoString &auth, AutoString &scopeId, AutoString &regId, AutoString &key);

    static void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen);
    static void readConnectionString(char * connectionString, uint32_t buffer_size);
    static void readIotCentralConfig(char * iotCentralConfig, uint32_t buffer_size);
    static void readGroupSXKeyAndDeviceId(char * scopeId, char * registrationId, char * sas, char &atype);

    static void clearWiFiEEPROM();
    static void clearAzureEEPROM();
    static void clearIotCentralEEPROM();
};
#endif /* CONFIG_H */
