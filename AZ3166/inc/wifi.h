// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef WIFI_H
#define WIFI_H

class WiFiController
{
    bool isConnected;
public:
    WiFiController():isConnected(false) { }

    bool getIsConnected() { return isConnected; }
    bool initApWiFi();
    bool initWiFi();

    void shutdownWiFi();
    void shutdownApWiFi();

    String * getWifiNetworks(int &count);
    void displayNetworkInfo();
};

#endif /* WIFI_H */