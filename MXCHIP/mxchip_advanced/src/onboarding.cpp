// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include <AZ3166WiFi.h>
#include <EEPROMInterface.h>

#include "../inc/onboarding.h"
#include "../inc/wifi.h"
#include "../inc/webServer.h"
#include "../inc/config.h"
#include "../inc/httpHtmlData.h"
#include "../inc/watchdogController.h"

static WatchdogController resetController;

void OnboardingController::initializeConfigurationSetup() {
    LOG_VERBOSE("OnboardingController::initializeConfigurationSetup");

    assert(initializeCompleted == false);

    // enter AP mode
    bool initWiFi = Globals::wiFiController.initApWiFi();
    LOG_VERBOSE("initApWifi: %d", initWiFi);

    if (initWiFi) {
        // setup web server
        assert(webServer == NULL);
        webServer = new AzWebServer();
        webServer->start();
        initializeCompleted = true;
    } else {
        LOG_ERROR("OnboardingController WiFi initialize has failed.");
    }
}

void OnboardingController::loop() {
    if (!initializeCompleted) {
        Screen.clean();
        Screen.print(0, "oopps..");
        Screen.print(1, "Unable to create");
        Screen.print(2, "a WiFi HotSpot.");
        Screen.print(3, "Press ('Reset')");
        delay(2000);
        return; // do not recall initializeConfigurationSetup here (stack-overflow)
    } else if (!setupCompleted) {
        Screen.clean();
        Screen.print(0, "Connect HotSpot:");
        Screen.print(1, Globals::wiFiController.getAPName());
        Screen.print(2, "go-> 192.168.0.1");
        char buffer[STRING_BUFFER_32] = {0};
        snprintf(buffer, STRING_BUFFER_32, "PIN CODE %s", Globals::wiFiController.getPassword());
        Screen.print(3, buffer);
    }

    assert(initializeCompleted == true);
    LOG_VERBOSE("initializeLoop: list for incoming clients");

    WiFiClient client = webServer->getClient();
    if (client)
    {
        LOG_VERBOSE("initializeLoop: new client");
        // an http request ends with a blank line
        boolean currentLineIsBlank = true;
        String request = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                request.concat(c);

                if (c == '\n' && currentLineIsBlank) {
                    String requestMethod = request.substring(0, request.indexOf("\r\n"));
                    requestMethod.toUpperCase();
                    if (requestMethod.startsWith("GET / ")) {
                        LOG_VERBOSE("Request to '/'");
                        client.write((uint8_t*)HTTP_MAIN_PAGE_RESPONSE, sizeof(HTTP_MAIN_PAGE_RESPONSE) - 1);
                        LOG_VERBOSE("Responsed with HTTP_MAIN_PAGE_RESPONSE");
                    } else if (requestMethod.startsWith("GET /START")) {
                        LOG_VERBOSE("-> request GET /START");
                        processStartRequest(client);
                    } else if (requestMethod.startsWith("GET /PROCESS")) {
                        LOG_VERBOSE("-> request GET /PROCESS");
                        processResultRequest(client, request);
                    } else if (requestMethod.startsWith("GET /COMPLETE")) {
                        LOG_VERBOSE("-> request GET /COMPLETE");
                        if (setupCompleted) {
                            Screen.clean();
                            Screen.print(0, "Completed!");
                            Screen.print(2, "Press 'reset'");
                            Screen.print(3, "         now :)");
                            client.write((uint8_t*)HTTP_COMPLETE_RESPONSE, sizeof(HTTP_COMPLETE_RESPONSE) - 1);
                            resetController.initialize(3000);
                            resetController.reset();
                        } else {
                            LOG_ERROR("User has landed on COMPLETE page without actually completing the setup. Writing the START page to client.");
                            processStartRequest(client);
                        }
                    } else {
                        // 404
                        LOG_VERBOSE("Request to %s -> 404!", request.c_str());
                        client.write((uint8_t*)HTTP_404_RESPONSE, sizeof(HTTP_404_RESPONSE) - 1);
                        LOG_VERBOSE("Responsed with HTTP_404_RESPONSE");
                    }
                    break;
                }
                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }

        // give the web browser time to receive the data
        delay(1);

        // close the connection:
        client.stop();
        LOG_VERBOSE("client disconnected");
    }
}

void OnboardingController::cleanup() {
    assert(initializeCompleted == false);

    initializeCompleted = false;
    if (webServer != NULL) {
        delete webServer;
        webServer = NULL;
    }
    Globals::wiFiController.shutdownApWiFi();
}

void OnboardingController::processStartRequest(WiFiClient &client) {
    int count = 0;
    String *networks = Globals::wiFiController.getWifiNetworks(count);
    if (networks == NULL) {
        LOG_ERROR("getWifiNetworks Out of Memory");
        client.write((uint8_t*)HTTP_ERROR_PAGE_RESPONSE, sizeof(HTTP_ERROR_PAGE_RESPONSE) - 1);
        return;
    }

    String networkOptions = "";
    for(int i = 0; i < count; i++) {
        networkOptions.concat("<option value=\"");
        networkOptions.concat(networks[i]);
        networkOptions.concat("\">");
        networkOptions.concat(networks[i]);
        networkOptions.concat("</option>");
    }
    delete [] networks;

    String startPageHtml = String(HTTP_START_PAGE_HTML);
    startPageHtml.replace("{{networks}}", networkOptions);
    client.write((uint8_t*)startPageHtml.c_str(), startPageHtml.length());
}

void OnboardingController::processResultRequest(WiFiClient &client, String &request) {
    String data = request.substring(request.indexOf('?') + 1, request.indexOf(" HTTP/"));
    const unsigned dataLength = data.length() + 1;
    char buff[dataLength] = {0};
    data.toCharArray(buff, dataLength);
    char *pch = strtok(buff, "&");
    StringBuffer ssid, password, auth, scopeId, regId, sasKey;
    uint8_t checkboxState = 0x00; // bit order - see globals.h

    while (pch != NULL)
    {
        String pair = String(pch);
        pair.trim();
        int idx = pair.indexOf("=");
        if (idx == -1) {
            LOG_ERROR("Broken Http Request. Responsed with HTTP_404_RESPONSE");
            client.write((uint8_t*)HTTP_404_RESPONSE, sizeof(HTTP_404_RESPONSE) - 1);
            return;
        }

        pch[idx] = char(0);
        const char* key = pch;
        const char * value = pair.c_str() + (idx + 1);
        unsigned valueLength = pair.length() - (idx + 1);
        bool unknown = false;

        switch(idx) {
            case 3:
            {
                if (strncmp(key, "HUM", 3) == 0) {
                    checkboxState = checkboxState | HUMIDITY_CHECKED;
                } else if (strncmp(key, "MAG", 3) == 0) {
                    checkboxState = checkboxState | MAG_CHECKED;
                } else {
                    unknown = true;
                }
            }
            break;
            case 4:
            {
                if (strncmp(key, "SSID", 4) == 0) {
                    urldecode(value, valueLength, &ssid);
                } else if (strncmp(key, "PASS", 4) == 0) {
                    urldecode(value, valueLength, &password);
                } else if (strncmp(key, "AUTH", 4) == 0) {
                    urldecode(value, valueLength, &auth);
                } else if (strncmp(key, "TEMP", 4) == 0) {
                    checkboxState = checkboxState | TEMP_CHECKED;
                } else if (strncmp(key, "PRES", 4) == 0) {
                    checkboxState = checkboxState | PRESSURE_CHECKED;
                } else if (strncmp(key, "GYRO", 4) == 0) {
                    checkboxState = checkboxState | GYRO_CHECKED;
                } else {
                    unknown = true;
                }
            }
            break;
            case 5:
            {
                if (strncmp(key, "ACCEL", 5) == 0) {
                    checkboxState = checkboxState | ACCEL_CHECKED;
                } else if (strncmp(key, "PINCO", 5) == 0) {
                    if (valueLength > 4 || strncmp(value, Globals::wiFiController.getPassword(), 4) != 0) {
                        LOG_ERROR("WRONG PIN CODE %s %s", value, Globals::wiFiController.getPassword());
                    } else {
                        pincodePasses = true;
                    }
                } else if (strncmp(key, "REGID", 5) == 0) {
                    urldecode(value, valueLength, &regId);
                } else {
                    unknown = true;
                }
            }
            break;

            default:
                if (idx == 6 && strncmp(key, "SASKEY", 6) == 0) {
                    urldecode(value, valueLength, &sasKey);
                } else if (idx == 7 && strncmp(key, "SCOPEID", 7) == 0) {
                    urldecode(value, valueLength, &scopeId);
                } else {
                    unknown = true;
                }
        }

        LOG_VERBOSE("Checkbox State %d", checkboxState);

        if (unknown) {
            LOG_ERROR("Unkown key:'%s' idx:'%d'", key, idx);
            LOG_ERROR("Unknown request parameter. Responsed with START page");
            processStartRequest(client);
            return;
        }

        pch = strtok(NULL, "&");
    }

    if (ssid.getLength() == 0 || ( *(*auth) == 'S' && sasKey.getLength() == 0) ) {
        LOG_ERROR("Missing ssid or connStr. Responsed with START page");
        processStartRequest(client);
        return;
    } else if (!pincodePasses) {
        client.write((uint8_t*)HTTP_REDIRECT_WRONG_PINCODE, sizeof(HTTP_REDIRECT_WRONG_PINCODE) - 1);
        return;
    }

    // store the settings in EEPROM
    assert(ssid.getLength() != 0);
    ConfigController::storeWiFi(ssid, password);

    assert(auth.getLength() != 0);
    ConfigController::storeKey(auth, scopeId, regId, sasKey);

    StringBuffer configData(3);
    snprintf(*configData, 3, "%d", checkboxState);
    ConfigController::storeIotCentralConfig(configData);

    setupCompleted = true;
    LOG_VERBOSE("Successfully processed the configuration request.");
    client.write((uint8_t*)HTTP_REDIRECT_RESPONSE, sizeof(HTTP_REDIRECT_RESPONSE) - 1);
}