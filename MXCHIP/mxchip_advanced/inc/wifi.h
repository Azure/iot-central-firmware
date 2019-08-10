// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef WIFI_H
#define WIFI_H

class WiFiUDP;
class WiFiController
{
    char apName[STRING_BUFFER_16];
    char macAddress[STRING_BUFFER_16];
    char password[4];
    bool isConnected;

public:
    WiFiController() : isConnected(false)
    {
    }
    bool initApWiFi();
    bool initApWiFi(char *pwd);
    bool initWiFi();

    void shutdownWiFi();
    void shutdownApWiFi();

    String *getWifiNetworks(int &count);
    void displayNetworkInfo();
    void getIPAddress(char *addr);
    void getBroadcastIp(char *braddress);
    void broadcastId();

    bool getIsConnected();

    const char *getAPName() { return apName; }
    const char *getPassword() { return password; }
};

#endif /* WIFI_H */