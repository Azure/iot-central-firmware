// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include <EEPROMInterface.h>

#include "../inc/pairing.h"
#include "../inc/wifi.h"
#include "../inc/webServer.h"
#include "../inc/config.h"
#include "../inc/httpHtmlData.h"
#include "../inc/watchdogController.h"

static WatchdogController resetController;

unsigned long previousMillis = 0; // will store last time LED was updated
int ledState = LOW;
// constants won't change:
const long interval = 100;

void PairingController::listen()
{
    LOG_VERBOSE("PairingController::listen");

    //assert(initializeCompleted == false);

    // enter AP mode
    bool initWiFi = Globals::wiFiController.initApWiFi();
    LOG_VERBOSE("initApWifi: %d", initWiFi);

    if (initWiFi)
    {
        udpClient.begin(4000);
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
         LOG_VERBOSE("reading socket");
        if (udpClient.read(triggerMessage, PAIRING_TRIGGER_LENGTH) > 0)
        {
            LOG_VERBOSE("Pairing message received: %s", triggerMessage);
            pair();
        }
        else
        {
            LOG_VERBOSE("Socket empty");
            delay(100);
        }
    }
}

void PairingController::pair()
{
    char buff[STRING_BUFFER_1024];
    const unsigned length = udpClient.read(buff, STRING_BUFFER_1024);
    if (length > 0)
    {
        char *pch = strtok(buff, ";");
        StringBuffer ssid, password, auth, scopeId, regId, sasKey;
        while (pch != NULL)
        {
            String pair = String(pch);
            pair.trim();
            int idx = pair.indexOf("=");
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
                memcpy(&ssid, value, valueLength);
            else if (idx == 4 && strncmp(key, "PASS", 4) == 0)
                memcpy(&password, value, valueLength);
            else if (idx == 6 && strncmp(key, "SASKEY", 6) == 0)
                memcpy(&sasKey, value, valueLength);
            else if (idx == 7 && strncmp(key, "SCOPEID", 7) == 0)
                memcpy(&scopeId, value, valueLength);
            else if (idx == 8 && strncmp(key, "DEVICEID", 8) == 0)
                memcpy(&regId, value, valueLength);
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
            pch = strtok(NULL, "&");
        }

        if (ssid.getLength() == 0)
        {
            LOG_ERROR("Missing ssid or connStr");
            return;
        }
        // else if (!pincodePasses)
        // {
        //     client.write((uint8_t *)HTTP_REDIRECT_WRONG_PINCODE, sizeof(HTTP_REDIRECT_WRONG_PINCODE) - 1);
        //     return;
        // }

        // store the settings in EEPROM
        assert(ssid.getLength() != 0);
        ConfigController::storeWiFi(ssid, password);
        ConfigController::storeKey(auth, scopeId, regId, sasKey);

        StringBuffer configData(3);
        snprintf(*configData, 3, "%d", 7);
        ConfigController::storeIotCentralConfig(configData);

        setupCompleted = true;
        LOG_VERBOSE("Successfully processed the configuration request.");
    }
}
void PairingController::cleanup()
{
    assert(initializeCompleted == false);

    initializeCompleted = false;
    Globals::wiFiController.shutdownApWiFi();
}
