// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include <AZ3166WiFi.h>
#include <EEPROMInterface.h>

#include "../inc/pairing.h"
#include "../inc/wifi.h"
#include "../inc/config.h"
#include "../inc/watchdogController.h"

static WatchdogController resetController;

unsigned long previousMillis = 0; // will store last time LED was updated
int ledState = LOW;
// constants won't change:
const long interval = 500;
StringBuffer ssid, password, auth, scopeId, regId, sasKey;
char address[STRING_BUFFER_16];
unsigned port;

void PairingController::listen()
{
    LOG_VERBOSE("PairingController::listen");
    // enter AP mode
    bool initWiFi = Globals::wiFiController.initApWiFi();
    LOG_VERBOSE("initApWifi: %d", initWiFi);
    if (initWiFi)
    {
        udpClient = new WiFiUDP();
        bool initSock = udpClient->begin(4000);
        LOG_VERBOSE("Initsock %d\n", initSock);
        initializeCompleted = true;
    }
    else
    {
        LOG_ERROR("Pairing WiFi initialize has failed.");
    }
}

void PairingController::loop()
{
    if (!initializeCompleted)
    {
        Screen.clean();
        Screen.print(0, "oopps..");
        Screen.print(1, "Unable to create");
        Screen.print(2, "a WiFi HotSpot.");
        Screen.print(3, "Press ('Reset')");
        delay(2000);
        return; // do not recall initializeConfigurationSetup here (stack-overflow)
    }
    else if (!setupCompleted)
    {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval)
        {
            // save the last time you blinked the LED
            previousMillis = currentMillis;

            // if the LED is off turn it on and vice-versa:
            if (ledState == LOW)
            {
                ledState = HIGH;
            }
            else
            {
                ledState = LOW;
            }
            // set the LED with the ledState of the variable:
            digitalWrite(LED_WIFI, ledState);
        }
        // int readln;
        // readln = udpClient->read(triggerMessage, PAIRING_TRIGGER_LENGTH);
        // if (readln > 0)
        // {
        //     LOG_VERBOSE("Pairing message received: %s, length: %d", triggerMessage, readln);
        //     if (strncmp(triggerMessage, "IOTC", readln) == 0)
        //     {
        //         Screen.clean();
        //         Screen.print(0, "Pairing with client started", true);
        //         pair();
        //         if (setupCompleted)
        //         {
        //             Screen.clean();
        //             Screen.print(0, "Completed!");
        //             Screen.print(2, "Press 'reset'");
        //             Screen.print(3, "         now :)");
        //         }
        //     }
        // }
        pair();
    }
}

void PairingController::pair()
{
    LOG_VERBOSE("Init pair");
    resetController.initialize(30000);
    if (!PairingController::startPairing())
    {
        errorAndReset();
        return;
    }
    if (!PairingController::receiveData())
    {
        errorAndReset();
        return;
    }
    if (!PairingController::storeConfig())
    {
        errorAndReset();
        return;
    }

    resetController.reset();
    delay(2000);
    setupCompleted = true;
    LOG_VERBOSE("Successfully processed the configuration request.");
    Screen.clean();
    Screen.print(0, "Pairing Completed!");
    Screen.print(1, "Restarting...");
    resetController.initialize(1500);
}
// void PairingController::pair()
// {
//     LOG_VERBOSE("Start pairing");
//     char buff[STRING_BUFFER_1024] = {0};
//     int length = 0;
//     while (length == 0)
//     {
//         resetController.reset();
//         LOG_VERBOSE("Waiting for data");

//         length = udpClient->read(buff, STRING_BUFFER_1024);
//         if (length > 0)
//         {
//             if (strncmp(buff, "IOTC", length) == 0)
//             {
//                 length = 0;
//                 continue;
//             }
//             Screen.clean();
//             Screen.print(0, "Credentials successfully received", true);

//             char data[length] = {0};
//             memcpy(data, buff, length);
//             char *pch = strtok(data, ";");
//             StringBuffer ssid, password, auth, scopeId, regId, sasKey;
//             while (pch != NULL)
//             {
//                 String pair = String(pch);
//                 // LOG_VERBOSE("Pair: %s", pair);
//                 pair.trim();
//                 int idx = pair.indexOf("=");
//                 if (idx == -1)
//                 {
//                     LOG_ERROR("Broken pairing message");
//                     return;
//                 }

//                 pch[idx] = char(0);
//                 const char *key = pch;
//                 const char *value = pair.c_str() + (idx + 1);
//                 unsigned valueLength = pair.length() - (idx + 1);
//                 bool unknown = false;

//                 if (idx == 4 && strncmp(key, "SSID", 4) == 0)
//                     urldecode(value, valueLength, &ssid);
//                 else if (idx == 4 && strncmp(key, "AUTH", 4) == 0)
//                     urldecode(value, valueLength, &auth);
//                 else if (idx == 4 && strncmp(key, "PASS", 4) == 0)
//                     urldecode(value, valueLength, &password);
//                 else if (idx == 6 && strncmp(key, "SASKEY", 6) == 0)
//                     urldecode(value, valueLength, &sasKey);
//                 else if (idx == 7 && strncmp(key, "SCOPEID", 7) == 0)
//                     urldecode(value, valueLength, &scopeId);
//                 else if (idx == 8 && strncmp(key, "DEVICEID", 8) == 0)
//                     urldecode(value, valueLength, &regId);
//                 else
//                 {
//                     unknown = true;
//                 }

//                 if (unknown)
//                 {
//                     LOG_ERROR("Unkown key:'%s' idx:'%d'", key, idx);
//                     LOG_ERROR("Unknown credentials parameter");
//                     return;
//                 }
//                 pch = strtok(NULL, ";");
//             }

//             if (ssid.getLength() == 0)
//             {
//                 LOG_ERROR("Missing ssid or connStr");
//                 return;
//             }

//             // store the settings in EEPROM
//             LOG_VERBOSE("Storing settings");
//             Screen.clean();
//             Screen.print(0, "Storing settings", true);
//             assert(ssid.getLength() != 0);
//             ConfigController::storeWiFi(ssid, password);
//             ConfigController::storeKey(auth, scopeId, regId, sasKey);

//             StringBuffer configData(3);
//             snprintf(*configData, 3, "%d", 7);
//             ConfigController::storeIotCentralConfig(configData);
//             resetController.reset();
//             delay(3000);
//             udpClient->stop();
//             delete udpClient;
//             udpClient = NULL;
//             Globals::wiFiController.shutdownApWiFi();
//             delay(2500);

//             if (Globals::wiFiController.initWiFi())
//             {
//                 Globals::wiFiController.displayNetworkInfo();

//                 if (broadcastSuccess())
//                 {
//                     setupCompleted = true;
//                     LOG_VERBOSE("Successfully processed the configuration request.");
//                     resetController.initialize(3000);
//                 }
//                 else
//                 {
//                     LOG_VERBOSE("Broadcasting failed");
//                     return; //restart pairing
//                 }
//             }
//             else
//             {
//                 LOG_VERBOSE("Can't connect to network. Restart");
//                 return;
//             }
//         }
//         else
//         {
//             LOG_VERBOSE("Data not available");
//             length = 0;
//         }
//     }
// }
void PairingController::cleanup()
{
    assert(initializeCompleted == false);

    initializeCompleted = false;
    Globals::wiFiController.shutdownApWiFi();
}

bool PairingController::startPairing()
{
    LOG_VERBOSE("Start pairing");
    int length = 0;

    char triggerMessage[PAIRING_TRIGGER_LENGTH] = {0};
    while (length <= 0)
    {
        LOG_VERBOSE("Reading");
        length = udpClient->read(triggerMessage, PAIRING_TRIGGER_LENGTH);
        if (length > 0)
        {
            LOG_VERBOSE("length %d\n", length);
            resetController.reset();
            LOG_VERBOSE("Pairing message received: %s, length: %d", triggerMessage, length);
            if (strncmp(triggerMessage, "IOTC", 4) == 0)
            {
                char *pch = strtok(triggerMessage, ":");
                short index = 0;
                while (pch != NULL)
                {
                    if (index == 1)
                        strncpy(address, pch, strlen(pch) + 1);
                    else if (index == 2)
                        port = atoi(pch);
                    pch = strtok(NULL, ":");
                    index++;
                }
                // send confirmation
                short tries = 0;
                char msgText[8] = "PAIRING";
                byte msg[8];
                memcpy(msg, msgText, strlen(msgText));
                while (tries < 5)
                {
                    udpClient->beginPacket(address, port);
                    udpClient->write(msg, strlen(msgText));
                    udpClient->endPacket();
                    tries++;
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        delay(50);
    }
}

bool PairingController::receiveData()
{
    char buff[STRING_BUFFER_1024];
    int length = 0;
    while (length <= 0)
    {
        LOG_VERBOSE("Waiting for data");
        length = udpClient->read(buff, STRING_BUFFER_1024);
        if (length > 0)
        {
            LOG_VERBOSE("Data received: %s", buff);
            resetController.reset();
            if (strncmp(buff, "IOTC", length) == 0)
            { // sender is trying to start pairing again. could be just a delay in the handshake. let's continue
                length = 0;
                continue;
            }
            Screen.clean();
            Screen.print(0, "Credentials successfully received", true);

            char data[length] = {0};
            memcpy(data, buff, length);
            char *pch = strtok(data, ";");
            while (pch != NULL)
            {
                String pair = String(pch);
                pair.trim();
                int idx = pair.indexOf("=");
                if (idx == -1)
                {
                    LOG_ERROR("Broken pairing message");
                    return false;
                }

                pch[idx] = char(0);
                const char *key = pch;
                const char *value = pair.c_str() + (idx + 1);
                unsigned valueLength = pair.length() - (idx + 1);
                bool unknown = false;

                if (idx == 4 && strncmp(key, "SSID", 4) == 0)
                    urldecode(value, valueLength, &ssid);
                else if (idx == 4 && strncmp(key, "AUTH", 4) == 0)
                    urldecode(value, valueLength, &auth);
                else if (idx == 4 && strncmp(key, "PASS", 4) == 0)
                    urldecode(value, valueLength, &password);
                else if (idx == 6 && strncmp(key, "SASKEY", 6) == 0)
                    urldecode(value, valueLength, &sasKey);
                else if (idx == 7 && strncmp(key, "SCOPEID", 7) == 0)
                    urldecode(value, valueLength, &scopeId);
                else if (idx == 8 && strncmp(key, "DEVICEID", 8) == 0)
                    urldecode(value, valueLength, &regId);
                else
                {
                    unknown = true;
                }

                if (unknown)
                {
                    LOG_ERROR("Unkown key:'%s' idx:'%d'", key, idx);
                    LOG_ERROR("Unknown credentials parameter");
                    return false;
                }
                pch = strtok(NULL, ";");
            }
            if (ssid.getLength() == 0)
            {
                LOG_ERROR("Missing ssid or connStr");
                return false;
            }
            short tries = 0;
            char msgText[9] = "RECEIVED";
            byte msg[9];
            memcpy(msg, msgText, strlen(msgText) + 1);
            while (tries < 5)
            {
                udpClient->beginPacket(address, port);
                udpClient->write(msg, strlen(msgText) + 1);
                udpClient->endPacket();
                tries++;
            }
            return true;
        }
        delay(50);
    }
}

bool PairingController::storeConfig()
{
    LOG_VERBOSE("Storing settings");
    resetController.initialize(5000);
    Screen.clean();
    Screen.print(0, "Storing settings", true);
    assert(ssid.getLength() != 0);
    ConfigController::storeWiFi(ssid, password);
    ConfigController::storeKey(auth, scopeId, regId, sasKey);

    StringBuffer configData(3);
    snprintf(*configData, 3, "%d", 7);
    ConfigController::storeIotCentralConfig(configData);
    resetController.reset();
    return true;
}

// bool PairingController::broadcastSuccess()
// {
//     LOG_VERBOSE("Stop current udp client");
//     udpClient->stop();
//     delete udpClient;
//     udpClient = NULL;
//     Globals::wiFiController.shutdownApWiFi();
//     delay(2500);

//     if (Globals::wiFiController.initWiFi())
//     {
//         Globals::wiFiController.displayNetworkInfo();
//         int tries = 0;
//         char res[2];
//         byte addr[16];
//         char *text = Globals::wiFiController.getIPAddress();
//         size_t length = strlen(text);
//         size_t msgLength;
//         memcpy(text, addr, length);
//         resetController.reset();
//         udpClient = new WiFiUDP();
//         udpClient->begin(4000);
//         while (tries < 5)
//         {

//             LOG_VERBOSE("Sending broadcast");
//             // LOG_VERBOSE("Begin packets");
//             udpClient->beginPacket("192.168.1.255", 9000);
//             udpClient->write(addr, length);
//             udpClient->endPacket();
//             delay(5);
//             msgLength = udpClient->read(res, 2);
//             if (msgLength > 0)
//             {
//                 LOG_VERBOSE("Received %s\n", res);
//                 if (strncmp(res, "OK", msgLength) == 0)
//                 {
//                     LOG_VERBOSE("Returning");
//                     return true;
//                 }
//             }
//             tries++;
//             // udpClient->stop();
//             delay(2000);
//         }
//         LOG_VERBOSE("Returning from broadcasting");
//         return false;
//     }

void PairingController::errorAndReset()
{
    Screen.clean();
    Screen.print(0, "Error during pairing. Resetting", true);
    resetController.reset();
    resetController.initialize(2000);
}
