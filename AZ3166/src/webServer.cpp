// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/webServer.h"

AzWebServer::AzWebServer() {
    webServer = NULL;
}

void AzWebServer::start() {
    assert(webServer == NULL);
    webServer = new WiFiServer(80);
    webServer->begin();
}

WiFiClient AzWebServer::getClient() {
    assert(webServer != NULL);
    return webServer->available();
}

void AzWebServer::stop() {
    assert(webServer != NULL);
    webServer->close();

    // TODO: Fix it
    // we need to talk to upstream people and fix deleting object of polymorphic class type 'WiFiServer'
    delete(webServer);

    webServer = NULL;
}