// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef WIFI_H
#define WIFI_H

class WiFiController
{
    char apName[STRING_BUFFER_16];
    char macAddress[STRING_BUFFER_16];
    char password[4];

    bool isConnected;
public:
    WiFiController():isConnected(false) { }

    bool initApWiFi();
    bool initWiFi();

    void shutdownWiFi();
    void shutdownApWiFi();

    String * getWifiNetworks(int &count);
    void displayNetworkInfo();

    bool getIsConnected();

    const char * getAPName() { return apName; }
    const char * getPassword() { return password; }
};

#endif /* WIFI_H */