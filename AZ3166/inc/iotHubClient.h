// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef IOT_HUB_CLIENT_H
#define IOT_HUB_CLIENT_H

typedef int (*hubMethodCallback)(const char *, size_t, char **response, size_t* resp_size);

#include <AzureIotHub.h>

typedef struct CALLBACK_LOOKUP_TAG {
    char* name;
    hubMethodCallback callback;
} CALLBACK_LOOKUP;

typedef struct EVENT_INSTANCE_TAG {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    int messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;

class IoTHubClient
{
    bool hasError;
    char deviceId[IOT_CENTRAL_MAX_LEN];
    char hubName[IOT_CENTRAL_MAX_LEN];
    int displayCharPos;
    int waitCount;
    bool needsCopying;
    char displayHubName[AZ3166_DISPLAY_MAX_COLUMN + 1];
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    int trackingId;

    void checkConnection() {
        if (needsReconnect) {
            // simple reconnection of the client in the event of a disconnect
            LOG_VERBOSE("Reconnecting to the IoT Hub");
            closeIotHubClient();
            initIotHubClient();

            needsReconnect = false;
        }
    }

    void hubClientYield(void) {
        checkConnection();

        IoTHubClient_LL_DoWork(iotHubClientHandle);
        ThreadAPI_Sleep(1 /* waitTime */);
    }

    void initIotHubClient();
    void closeIotHubClient();

public:
    IoTHubClient(): hasError(false), displayCharPos(0),
                    waitCount(3), needsCopying(true),
                    iotHubClientHandle(NULL), trackingId(0),
                    methodCallbackCount(0), desiredCallbackCount(0),
                    needsReconnect(false)
    {
        memset(deviceId, 0, IOT_CENTRAL_MAX_LEN);
        memset(hubName,  0, IOT_CENTRAL_MAX_LEN);
        initIotHubClient();
    }

    ~IoTHubClient()
    {
        closeIotHubClient();
    }

    bool wasInitialized() {
        return !hasError;
    }

    bool sendTelemetry(const char *payload);
    bool sendReportedProperty(const char *payload);

    bool registerMethod(const char *methodName, hubMethodCallback callback);
    bool registerDesiredProperty(const char *propertyName, hubMethodCallback callback);

    void displayDeviceInfo(); // TODO: should this go under device?

    int methodCallbackCount;
    int desiredCallbackCount;
    CALLBACK_LOOKUP methodCallbackList[MAX_CALLBACK_COUNT];
    CALLBACK_LOOKUP desiredCallbackList[MAX_CALLBACK_COUNT];
    bool needsReconnect;
};

#endif /* IOT_HUB_CLIENT_H */