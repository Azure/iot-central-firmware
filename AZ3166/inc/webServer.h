// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

bool startWebServer();
WiFiClient clientAvailable();
String getRequest(WiFiClient client);
String getPostBody(String request, WiFiClient client);
bool stopWebServer();

#endif /* WEB_SERVER_H */