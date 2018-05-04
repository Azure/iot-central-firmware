// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/webServer.h"

AzWebServer::AzWebServer() {
    server = NULL;
}

void AzWebServer::start() {
    assert(server == NULL);
    server = new WiFiServer(80);
    server->begin();
}

WiFiClient AzWebServer::getClient() {
    assert(server != NULL);
    return server->available();
}

void AzWebServer::stop() {
    assert(server != NULL);
    server->close();

    // TODO: Fix it
    // we need to talk to upstream people and fix deleting object of polymorphic class type 'WiFiServer'
    delete(server);

    server = NULL;
}