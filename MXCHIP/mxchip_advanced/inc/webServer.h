// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "AZ3166WiFi.h"
#include "Arduino.h"

class AzWebServer {
  WiFiServer *server;

 public:
  AzWebServer();
  void start();
  WiFiClient getClient();
  void stop();
};

#endif /* WEB_SERVER_H */