// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include "AZ3166WiFi.h"
#include "EEPROMInterface.h"

#include "../inc/mainInitialize.h"
#include "../inc/wifi.h"
#include "../inc/webServer.h"
#include "../inc/config.h"
#include "../inc/utility.h"
#include "../inc/httpHtmlData.h"

// forward declarations
void processResultRequest(WiFiClient client, String request);
void processStartRequest(WiFiClient client);

void initializeSetup() {
    assert(Globals::needsInitialize == true);
    Globals::needsInitialize = false;

    // enter AP mode
    bool apRunning = initApWiFi();
    Serial.printf("initApWifi: %d \r\n", apRunning);

    // setup web server
    Globals::webServer.start();
}

void initializeLoop() {
    // if we are about to reset then stop processing any requests
    if (Globals::needsInitialize) {
        delay(1);
        return;
    }

    Serial.println("initializeLoop: list for incoming clients");

    WiFiClient client = Globals::webServer.getClient();
    if (client) // ( _pTcpSocket != NULL )
    {
        Serial.println("initializeLoop: new client");
        // an http request ends with a blank line
        boolean currentLineIsBlank = true;
        String request = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);
                request.concat(c);

                if (c == '\n' && currentLineIsBlank) {
                    String requestMethod = request.substring(0, request.indexOf("\r\n"));
                    requestMethod.toUpperCase();
                    if (requestMethod.startsWith("GET / ")) {
                        Serial.printf("Request to '/'\r\n");
                        client.write((uint8_t*)HTTP_MAIN_PAGE_RESPONSE, sizeof(HTTP_MAIN_PAGE_RESPONSE) - 1);
                        Serial.println("Responsed with HTTP_MAIN_PAGE_RESPONSE");
                    } else if (requestMethod.startsWith("GET /START")) {
                        Serial.println("-> request GET /START");
                        processStartRequest(client);
                    } else if (requestMethod.startsWith("GET /PROCESS")) {
                        Serial.println("-> request GET /PROCESS");
                        processResultRequest(client, request);
                    } else if (requestMethod.startsWith("GET /COMPLETE")) {
                        Serial.println("-> request GET /COMPLETE");
                        client.write((uint8_t*)HTTP_COMPLETE_RESPONSE, sizeof(HTTP_COMPLETE_RESPONSE) - 1);
                    } else {
                        // 404
                        Serial.printf("Request to %s -> 404!\r\n", request.c_str());
                        client.write((uint8_t*)HTTP_404_RESPONSE, sizeof(HTTP_404_RESPONSE) - 1);
                        Serial.println("Responsed with HTTP_404_RESPONSE");
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
        Serial.println("client disconnected");
    }
}

void initializeCleanup() {
    Globals::needsInitialize = true;
    Globals::webServer.stop();
    shutdownApWiFi();
}

void processStartRequest(WiFiClient client) {
    int count = 0;
    String *networks = getWifiNetworks(count);
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

void processResultRequest(WiFiClient client, String request) {
    String data = request.substring(request.indexOf('?') + 1, request.indexOf(" HTTP/"));
    const unsigned dataLength = data.length() + 1;
    char buff[dataLength];
    data.toCharArray(buff, dataLength);
    char *pch = strtok(buff, "&");
    String ssid = "";
    String password = "";
    String connStr = "";
    uint8_t checkboxState = 0x00; // bit order - TEMP, HUMIDITY, PRESSURE, ACCELEROMETER, GYROSCOPE, MAGNETOMETER

    while (pch != NULL)
    {
        String pair = String(pch);
        pair.trim();
        int idx = pair.indexOf("=");
        String key = pair.substring(0, idx);
        String value = pair.substring(idx+1);

        if (key == "SSID") {
            ssid = urldecode(value);
        } else if (key == "PASS") {
            password = urldecode(value);
        } else if (key == "CONN") {
            connStr = urldecode(value);
        } else if (key == "TEMP") {
            checkboxState = checkboxState | 0x80;
        } else if (key == "HUM") {
            checkboxState = checkboxState | 0x40;
        } else if (key == "PRES") {
            checkboxState = checkboxState | 0x20;
        } else if (key == "ACCEL") {
            checkboxState = checkboxState | 0x10;
        } else if (key == "GYRO") {
            checkboxState = checkboxState | 0x08;
        } else if (key == "MAG") {
            checkboxState = checkboxState | 0x04;
        }

        pch = strtok(NULL, "&");
    }

    // store the settings in EEPROM
    storeWiFi(ssid.c_str(), password.c_str());
    storeConnectionString(connStr.c_str());
    char configData[4];
    int configLength = snprintf(configData, 4, "!#%c", checkboxState);
    configData[configLength] = 0;
    storeIotCentralConfig(configData, 3);

    Serial.println("Successfully processed the configuration request.");
    client.write((uint8_t*)HTTP_REDIRECT_RESPONSE, sizeof(HTTP_REDIRECT_RESPONSE) - 1);
}