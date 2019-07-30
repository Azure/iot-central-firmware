// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

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
        initializeCompleted = true;
    }
    else
    {
        LOG_ERROR("Pairing WiFi initialize has failed.");
    }
}

void PairingController::loop()
{
    // LOG_VERBOSE("here");
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
        LOG_VERBOSE("reading socket");
        int readln;
        readln = udpClient->read(triggerMessage, PAIRING_TRIGGER_LENGTH);
        if (readln > 0)
        {
            LOG_VERBOSE("Pairing message received: %s, length: %d", triggerMessage, readln);
            if (strncmp(triggerMessage, "IOTC", readln) == 0)
            {
                LOG_VERBOSE("Got pairing message");
                pair();
                if (setupCompleted)
                {
                    Screen.clean();
                    Screen.print(0, "Completed!");
                    Screen.print(2, "Press 'reset'");
                    Screen.print(3, "         now :)");
                    resetController.initialize(3000);
                    resetController.reset();
                }
            }
        }
        else
        {
            LOG_VERBOSE("Socket empty");
        }
    }
}

void PairingController::pair()
{
    LOG_VERBOSE("Start pairing");
    char buff[STRING_BUFFER_1024];
    int length = 0;
    while (length == 0)
    {
        LOG_VERBOSE("Waiting for data");

        length = udpClient->read(buff, STRING_BUFFER_1024);
        if (length > 0)
        {
            LOG_VERBOSE("Got data: %s length:%d. Cleaning buffer", buff, length);
            char data[length] = {0};
            memcpy(data, buff, length);
            char *pch = strtok(data, ";");
            StringBuffer ssid, password, auth, scopeId, regId, sasKey;
            while (pch != NULL)
            {
                String pair = String(pch);
                // LOG_VERBOSE("Pair: %s", pair);
                pair.trim();
                int idx = pair.indexOf("=");
                LOG_VERBOSE("Indice %d", idx);
                if (idx == -1)
                {
                    LOG_ERROR("Broken pairing message");
                    return;
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
                    return;
                }
                pch = strtok(NULL, ";");
            }

            if (ssid.getLength() == 0)
            {
                LOG_ERROR("Missing ssid or connStr");
                return;
            }

            // store the settings in EEPROM
            LOG_VERBOSE("Storing settings");
            assert(ssid.getLength() != 0);
            ConfigController::storeWiFi(ssid, password);
            ConfigController::storeKey(auth, scopeId, regId, sasKey);

            StringBuffer configData(3);
            snprintf(*configData, 3, "%d", 7);
            ConfigController::storeIotCentralConfig(configData);

            setupCompleted = true;
            LOG_VERBOSE("Successfully processed the configuration request.");
        }
        else
        {
            length = 0;
        }
    }
}
void PairingController::cleanup()
{
    assert(initializeCompleted == false);

    initializeCompleted = false;
    Globals::wiFiController.shutdownApWiFi();
}
