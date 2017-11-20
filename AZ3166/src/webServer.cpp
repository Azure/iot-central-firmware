// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"
#include "AZ3166WiFi.h"

#include "../inc/webServer.h"

WiFiServer *webServer;

bool startWebServer() {
    webServer = new WiFiServer(80);
    webServer->begin();
}

WiFiClient clientAvailable() {
    return webServer->available();
}

bool stopWebServer() {
    webServer->close();
    delete(webServer);
}