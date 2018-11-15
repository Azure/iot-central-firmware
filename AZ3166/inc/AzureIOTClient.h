// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOT_CLIENT_H
#define AZURE_IOT_CLIENT_H

typedef int (*hubMethodCallback)(const char *, size_t);

#include <AzureIotHub.h>
#include "iotc.h"

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
        methodName(NULL), payload(NULL), length(0), next(NULL) { }
    DirectMethodNode(char *m, char *p, size_t s):
        methodName(m), payload(p), length(s), next(NULL) { }
};

class AzureIOTClient
{
    IOTContext context;

    bool hasError;
    int displayCharPos;
    int waitCount;

    DirectMethodNode *rootNode, *lastNode;

    void init();
    void close();

public:
    AzureIOTClient(): context(NULL), hasError(false), displayCharPos(0),
                    rootNode(NULL), lastNode(NULL), methodCallbackCount(0),
                    desiredCallbackCount(0), needsReconnect(false)
    {
        init();
    }

    ~AzureIOTClient()
    {
        close();
    }

    void hubClientYield(void) {
        checkConnection();

        iotc_do_work(context);
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
            if (!tmp) {
                LOG_ERROR("tmp shouldn't be NULL");
                return NULL;
            }
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
            close();
            init();

            needsReconnect = false;
        }
    }

    bool sendTelemetry(const char *payload);
    bool sendReportedProperty(const char *payload);

    bool registerMethod(const char *methodName, hubMethodCallback callback);
    bool registerDesiredProperty(const char *propertyName, hubMethodCallback callback);

    void displayDeviceInfo(); // TODO: should this go under device?

    int methodCallbackCount;
    int desiredCallbackCount;
    CALLBACK_LOOKUP_M methodCallbackList[MAX_CALLBACK_COUNT];
    CALLBACK_LOOKUP_M desiredCallbackList[MAX_CALLBACK_COUNT];
    bool needsReconnect;
};

#endif /* AZURE_IOT_CLIENT_H */