// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef IOT_HUB_CLIENT_H
#define IOT_HUB_CLIENT_H

typedef int (*hubDesiredCallback)(const char *, size_t, char **response, size_t* resp_size);
typedef void (*hubMethodCallback)(const char *, size_t);

#include <AzureIotHub.h>

typedef struct CALLBACK_LOOKUP_TAG_D {
    char* name;
    hubDesiredCallback callback;
} CALLBACK_LOOKUP_D;

typedef struct CALLBACK_LOOKUP_TAG_M {
    char* name;
    hubMethodCallback callback;
} CALLBACK_LOOKUP_M;

typedef struct EVENT_INSTANCE_TAG {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    int messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;

struct DirectMethodNode
{
    char *methodName;
    char *payload;
    size_t length;

    DirectMethodNode *next;
    DirectMethodNode():
        methodName(NULL), payload(NULL), next(NULL), length(0) { }
    DirectMethodNode(char *m, char *p, size_t s):
        methodName(m), payload(p), next(NULL), length(s) { }
};

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

    DirectMethodNode *rootNode, *lastNode;

    void initIotHubClient();
    void closeIotHubClient();

public:
    IoTHubClient(): hasError(false), displayCharPos(0),
                    waitCount(3), needsCopying(true),
                    iotHubClientHandle(NULL), trackingId(0), rootNode(NULL),
                    lastNode(NULL), methodCallbackCount(0),
                    desiredCallbackCount(0), needsReconnect(false)
    {
        memset(deviceId, 0, IOT_CENTRAL_MAX_LEN);
        memset(hubName,  0, IOT_CENTRAL_MAX_LEN);
        initIotHubClient();
    }

    ~IoTHubClient()
    {
        closeIotHubClient();
    }

    void hubClientYield(void) {
        checkConnection();

        IoTHubClient_LL_DoWork(iotHubClientHandle);
        ThreadAPI_Sleep(1 /* waitTime */);
    }

    void pushDirectMethod(char *method, char *payload, size_t size) {
        if (!rootNode) {
            rootNode = new DirectMethodNode(method, payload, size);
            lastNode = rootNode;
        } else {
            lastNode->next = new DirectMethodNode(method, payload, size);
            lastNode = lastNode->next;
        }
    }

    DirectMethodNode * popDirectMethod() {
        if (!lastNode) {
            return NULL;
        }

        DirectMethodNode *tmp = rootNode;
        if (lastNode == rootNode) {
            rootNode = NULL;
            lastNode = NULL;
            return tmp;
        }

        while(tmp->next != lastNode) {
            tmp = tmp->next;
            assert(tmp);
        }

        lastNode = tmp;
        tmp = tmp->next;
        lastNode->next = NULL;

        return tmp;
    }

    void freeDirectMethod(DirectMethodNode *node) {
        free(node->methodName);
        free(node->payload);
        memset(node, 0, sizeof(DirectMethodNode));
        delete node;
    }

    bool wasInitialized() {
        return !hasError;
    }

    void checkConnection() {
        if (needsReconnect) {
            // simple reconnection of the client in the event of a disconnect
            LOG_VERBOSE("Reconnecting to the IoT Hub");
            closeIotHubClient();
            initIotHubClient();

            needsReconnect = false;
        }
    }

    bool sendTelemetry(const char *payload);
    bool sendReportedProperty(const char *payload);

    bool registerMethod(const char *methodName, hubMethodCallback callback);
    bool registerDesiredProperty(const char *propertyName, hubDesiredCallback callback);

    void displayDeviceInfo(); // TODO: should this go under device?

    int methodCallbackCount;
    int desiredCallbackCount;
    CALLBACK_LOOKUP_M methodCallbackList[MAX_CALLBACK_COUNT];
    CALLBACK_LOOKUP_D desiredCallbackList[MAX_CALLBACK_COUNT];
    bool needsReconnect;
};

#endif /* IOT_HUB_CLIENT_H */