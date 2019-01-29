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

    static void storeWiFi(StringBuffer &ssid, StringBuffer &password);
    static void storeConnectionString(StringBuffer &connectionString);
    static void storeIotCentralConfig(StringBuffer &iotCentralConfig);
    static void storeKey(StringBuffer &auth, StringBuffer &scopeId, StringBuffer &regId, StringBuffer &key);

    static void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen);
    static void readConnectionString(char * connectionString, uint32_t buffer_size);
    static void readIotCentralConfig(char * iotCentralConfig, uint32_t buffer_size);
    static void readGroupSXKeyAndDeviceId(char * scopeId, char * registrationId, char * sas, char &atype);

    static void clearWiFiEEPROM();
    static void clearAzureEEPROM();
    static void clearIotCentralEEPROM();
};
#endif /* CONFIG_H */
