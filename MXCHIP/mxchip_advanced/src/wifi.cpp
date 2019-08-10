// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include <AZ3166WiFi.h>
#include <AZ3166WiFiUdp.h>
#include "../inc/wifi.h"

#include "../inc/config.h"
#include "../inc/watchdogController.h"

static WatchdogController resetController;


bool WiFiController::initApWiFi()
{
    char pwd[] = "";
    return WiFiController::initApWiFi(pwd);
}

bool WiFiController::initApWiFi(char *pwd)
{
    LOG_VERBOSE("WiFiController::initApWiFi");

    byte mac[6] = {0};
    WiFi.macAddress(mac);
    unsigned length = snprintf(apName, STRING_BUFFER_32 - 1, "AZ3166_%c%c%c%c%c%c",
                               mac[0] % 26 + 65, mac[1] % 26 + 65, mac[2] % 26 + 65, mac[3] % 26 + 65,
                               mac[4] % 26 + 65, mac[5] % 26 + 65);
    apName[length] = char(0);

    length = snprintf(macAddress, STRING_BUFFER_16, "%02X%02X%02X%02X%02X%02X",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    macAddress[length] = char(0);
    LOG_VERBOSE("MAC address %s", macAddress);

    memcpy(password, macAddress + 8, 4);
    password[4] = 0;

    // passcode below is not widely compatible. See the password is used as pincode for onboarding
    int ret = WiFi.beginAP(apName, pwd);

    if (ret != WL_CONNECTED)
    {
        Screen.print(0, "AP Failed:");
        Screen.print(2, "Reboot device");
        Screen.print(3, "and try again");
        LOG_ERROR("AP creation failed");
        return false;
    }
    LOG_VERBOSE("AP started");
    return true;
}

bool WiFiController::initWiFi()
{
    LOG_VERBOSE("WiFiController::initWiFi");
    Screen.clean();
    Screen.print("WiFi \r\n \r\nConnecting...");

    if (WiFi.begin() == WL_CONNECTED)
    {
        LOG_VERBOSE("WiFi WL_CONNECTED");
        digitalWrite(LED_WIFI, 1);
        isConnected = true;
    }
    else
    {
        Screen.print("WiFi\r\nNot Connected\r\nEnter AP Mode?\r\n");
    }

    return isConnected;
}

void WiFiController::shutdownWiFi()
{
    LOG_VERBOSE("WiFiController::shutdownWiFi");
    WiFi.disconnect();
    isConnected = false;
}

void WiFiController::shutdownApWiFi()
{
    LOG_VERBOSE("WiFiController::shutdownApWiFi");
    WiFi.disconnectAP();
    isConnected = false;
}

bool WiFiController::getIsConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String *WiFiController::getWifiNetworks(int &count)
{
    LOG_VERBOSE("WiFiController::getWifiNetworks");
    String foundNetworks = ""; // keep track of network SSID so as to remove duplicates from mesh and repeater networks
    int numSsid = WiFi.scanNetworks();
    count = 0;

    if (numSsid == -1)
    {
        return NULL;
    }
    else
    {
        String *networks = new String[numSsid + 1]; // +1 if all unique
        if (networks == NULL /* unlikely */)
        {
            return NULL;
        }

        if (numSsid > 0)
        {
            networks[0] = WiFi.SSID(0);
            count = 1;
        }

        // TODO: can we make this better?
        for (int thisNet = 1; thisNet < numSsid; thisNet++)
        {
            const char *ssidName = WiFi.SSID(thisNet);
            if (strlen(ssidName) == 0)
                continue;

            networks[count] = ssidName; // prefill
            int lookupPosition = 0;
            for (; lookupPosition < count; lookupPosition++)
            {
                if (networks[count] == networks[lookupPosition])
                    break;
            }
            if (lookupPosition < count)
            { // ditch if found
                continue;
            }
            count++; // save if not found
        }

        return networks;
    }
}

void WiFiController::displayNetworkInfo()
{
    LOG_VERBOSE("WiFiController::displayNetworkInfo");
    char buff[STRING_BUFFER_128] = {0};
    IPAddress ip = WiFi.localIP();
    byte mac[6] = {0};

    WiFi.macAddress(mac);
    unsigned length = snprintf(buff, STRING_BUFFER_128,
                               "WiFi:\r\n%s\r\n%s\r\nmac:%02X%02X%02X%02X%02X%02X",
                               WiFi.SSID(), ip.get_address(),
                               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    buff[length] = char(0);
    Screen.print(0, buff);
}

void WiFiController::getIPAddress(char *address)
{
    LOG_VERBOSE("WiFiController::getIPAddress");
    IPAddress ip = WiFi.localIP();
    strcpy(address, ip.get_address());
}

void WiFiController::getBroadcastIp(char *address)
{
    IPAddress broadcastIp;
    broadcastIp = WiFi.localIP() | (~WiFi.subnetMask());
    strcpy(address, broadcastIp.get_address());
}

// void WiFiController::broadcastId()
// {
//     byte mac[6] = {0};
//     WiFi.macAddress(mac);
//     char id[7];
//     int length = snprintf(id, 6, "%c%c%c%c%c%c",
//                           mac[0] % 26 + 65, mac[1] % 26 + 65, mac[2] % 26 + 65, mac[3] % 26 + 65,
//                           mac[4] % 26 + 65, mac[5] % 26 + 65);
//     char msg[STRING_BUFFER_32] = {0};
//     char ipAddr[STRING_BUFFER_16] = {0};
//     WiFiController::getIPAddress(ipAddr);
//     int msgLength = snprintf(msg, STRING_BUFFER_32, "%s:%s", id, ipAddr);
//     LOG_VERBOSE("Message length %d", msgLength);
//     byte buf[msgLength];
//     memcpy(buf, msg, msgLength);
//     LOG_VERBOSE("Broadcast %s", (char *)buf);
//     char broadcastAddr[STRING_BUFFER_16] = {0};
//     WiFiController::getBroadcastIp(broadcastAddr);

//     short tries = 0;
//     while (tries < 5)
//     {
//         resetController.reset();
//         LOG_VERBOSE("Sending to %s", broadcastAddr);
//         udpClient->beginPacket("192.168.1.255", 9000);
//         udpClient->write(buf, msgLength);
//         udpClient->endPacket();
//         tries++;
//         delay(500);
//     }
//     LOG_VERBOSE("stop sending");

//     // udpClient->stop();
//     // delete udpClient;
// }

void WiFiController::broadcastId()
{
    // char ipAddr[STRING_BUFFER_16] = {0};
    // WiFiController::getIPAddress(ipAddr);
    // LOG_VERBOSE("IP: %s", ipAddr);
    short tries = 0;
    while (tries < 5)
    {
        Globals::udpClient->beginPacket("192.168.1.255", 9000);
        Globals::udpClient->write((const unsigned char *)"pippo", 6);
        Globals::udpClient->endPacket();
        tries++;
    }
    LOG_VERBOSE("Finito");
}