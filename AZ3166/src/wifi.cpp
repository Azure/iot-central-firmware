// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"
#include "AZ3166WiFi.h"

#include "../inc/config.h"

bool initApWiFi() {
    char ap_name[14];
    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(ap_name, "AZ3166_%c%c%c%c%c%c", mac[0] % 26 + 65, mac[1]% 26 + 65, mac[2]% 26 + 65, mac[3]% 26 + 65, mac[4]% 26 + 65, mac[5]% 26 + 65);
    char macAddress[18];
    sprintf(macAddress, "mac:%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    int ret = WiFi.beginAP(ap_name, "");

    Screen.print(0, "WiFi name:");
    Screen.print(1, ap_name);
    Screen.print(2, macAddress);
    Screen.print(3, "Ready to connect");

    if ( ret != WL_CONNECTED) {
      Screen.print(0, "AP Failed:");
      Screen.print(2, "Reboot device");
      Screen.print(3, "and try again");
      Serial.println("AP creation failed");
      return false;
    }
    Serial.println("AP started");
    return true;
}

bool initWiFi() {
    bool connected = false;
    char wifiBuff[128];

    Screen.print("WiFi \r\n \r\nConnecting...\r\n             \r\n");
    
    if(WiFi.begin() == WL_CONNECTED) {
        digitalWrite(LED_WIFI, 1);
        connected = true;
    }

    return connected;
}

void shutdownWiFi() {
    WiFi.disconnect();
}

void shutdownApWiFi() {
    WiFi.disconnectAP();
}

String * getWifiNetworks(int &count) {
    String foundNetworks = "";  // keep track of network SSID so as to remove duplicates from mesh and repeater networks 
    int numSsid = WiFi.scanNetworks();
    if (numSsid == -1) {
      count = 0;
      return NULL;
    } else {
        String *tempNetworks = new String[numSsid];
        count = 0;
        for (int thisNet = 0; thisNet < numSsid; thisNet++) {
            String lookupNetwork;
            lookupNetwork.concat(WiFi.SSID(thisNet));
            lookupNetwork.concat(",");
            if (foundNetworks.indexOf(lookupNetwork) > -1) {
                continue;
            }
            foundNetworks = foundNetworks + WiFi.SSID(thisNet) + ",";
            tempNetworks[count] = String(WiFi.SSID(thisNet));
            count++;
        }

        String *networks = new String[count];
        for (int i = 0; i < count; i++) {
            networks[i] = tempNetworks[i];
        }
        delete [] tempNetworks;

        return networks;
    } 
}

void displayNetworkInfo() {
    char buff[64];
    IPAddress ip = WiFi.localIP();
    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(buff, "WiFi:\r\n%s\r\n%s\r\mac:%02X%02X%02X%02X%02X%02X", WiFi.SSID(), ip.get_address(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Screen.print(0, buff);
}
