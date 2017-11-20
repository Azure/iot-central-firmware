// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef WIFI_H
#define WIFI_H

bool initApWiFi();
bool initWiFi();

void shutdownApWiFi();
void shutdownWiFi();

void displayNetworkInfo();

String * getWifiNetworks(int &count);

#endif /* WIFI_H */