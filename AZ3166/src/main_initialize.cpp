// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"
#include "AZ3166WiFi.h"
#include "EEPROMInterface.h"

#include "../inc/main_initialize.h"
#include "../inc/wifi.h"
#include "../inc/webServer.h"
#include "../inc/config.h"
#include "../inc/utility.h"
#include "../inc/httpHtmlData.h"

// forward declarations
void processResultRequest(WiFiClient client, String request);
void processStartRequest(WiFiClient client);

static bool reset = false;

void initializeSetup() {

    reset = false;

    // enter AP mode
    bool apRunning = initApWiFi();

    // setup web server
    bool webServerRunning = startWebServer();
}

void initializeLoop() {

    // if we are about to reset then stop processing any requests
    if (reset) {
        delay(1);
        return;
    }

    Serial.println("list for incoming clients");
    // listen for incoming clients
    WiFiClient client = clientAvailable();
    Serial.println("availabled");
    if (client) 
    {
        Serial.println("new client");
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
                    if (requestMethod.startsWith("GET /START")) {
                        Serial.println("-> request GET /START");
                        processStartRequest(client);
                    } else if (requestMethod.startsWith("GET /RESULT")) {
                        Serial.println("-> request GET /RESULT");
                        processResultRequest(client, request);
                    } else if (requestMethod.startsWith("GET /COMPLETE")) {
                        Serial.println("-> request GET /COMPLETE");
                        String response = String(HTTP_STATUS_200) + httpHeaderNocache + completePageHtml + "\r\n";
                        client.write((uint8_t*)response.c_str(), strlen(response.c_str()));
                    } else {
                        // 404
                        Serial.print("-> 404: ");
                        Serial.println(request);
                        String response = String(HTTP_STATUS_404) + httpHeaderNocache;
                        client.write((uint8_t*)response.c_str(), strlen(response.c_str()));
                        Serial.println(request);
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
    reset = true;
    stopWebServer();
    shutdownApWiFi();
}

void processStartRequest(WiFiClient client) {
    int count;
    String *networks = getWifiNetworks(count);
    String networkOptions = "";
    for(int i = 0; i < count; i++) {
        networkOptions.concat("<option value=\"");
        networkOptions.concat(networks[i]);
        networkOptions.concat("\">");
        networkOptions.concat(networks[i]);
        networkOptions.concat("</option>");
    }

    startPageHtml.replace("{{networks}}", networkOptions);
    String response = String(HTTP_STATUS_200) + httpHeaderNocache + startPageHtml + "\r\n";
    client.write((uint8_t*)response.c_str(), strlen(response.c_str()));
}

void processResultRequest(WiFiClient client, String request) {
    String data = request.substring(request.indexOf('?') + 1, request.indexOf(" HTTP/"));
    char buff[data.length()+1];
    data.toCharArray(buff, data.length()+1);
    char *pch = strtok(buff,"&");
    String ssid = "";
    String password = "";
    String connStr = "";
    uint8_t checkboxState = 0x00; // bit order - TEMP, HUMIDITY, PRESSURE, ACCELEROMETER, GYROSCOPE, MAGNETOMETER
    int error = 0;

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
    sprintf(configData, "!#%c", checkboxState);
    storeIotCentralConfig(configData, 3);

    // redirect to the complete page
    String response = String(HTTP_STATUS_302) + "\r\nLocation: /complete\r\n\r\n\r\n";

    client.write((uint8_t*)response.c_str(), strlen(response.c_str()));
}