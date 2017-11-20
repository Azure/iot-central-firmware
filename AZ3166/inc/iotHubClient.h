// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

typedef int (*methodCallback)(const char *, size_t, char **response, size_t* resp_size);

#ifndef IOT_HUB_CLIENT_H
#define IOT_HUB_CLIENT_H

void initIotHubClient(bool traceOn);
bool sendTelemetry(const char *payload);
bool sendReportedProperty(const char *payload);

bool registerMethod(const char *methodName, methodCallback callback);
bool registerDesiredProperty(const char *propertyName, methodCallback callback);

void closeIotHubClient(void);

void displayDeviceInfo();

#endif /* IOT_HUB_CLIENT_H */