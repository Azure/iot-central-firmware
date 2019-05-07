// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOTC_INTERNAL_H
#define AZURE_IOTC_INTERNAL_H

#include "iotc_definitions.h"
#include "iotc_platform.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>  // size_t etc.
#include "string_buffer.h"

#include "../iotc.h"

#define AZ_IOT_HUB_MAX_LEN 1024
#define DEFAULT_ENDPOINT "global.azure-devices-provisioning.net"
#define TO_STR_(s) #s
#define TO_STR(s) TO_STR_(s)

typedef enum IOTCallbacks_TAG {
  ConnectionStatus = 0x01,
  MessageSent,
  Command,
  Error,
  SettingsUpdated
} IOTCallbacks;

typedef struct CallbackBase_TAG {
  IOTCallback callback;
  void *appContext;
  CallbackBase_TAG() {
    callback = NULL;
    appContext = NULL;
  }
} CallbackBase;

typedef struct IOTContextInternal_TAG {
  char *endpoint;
  char *modelData;
  IOTProtocol protocol;
  CallbackBase callbacks[8];

  int messageId;
  AzureIOT::StringBuffer deviceId;
  ARDUINO_WIFI_SSL_CLIENT *tlsClient;
  PubSubClient *mqttClient;
} IOTContextInternal;

#define CHECK_NOT_NULL(x)             \
  if (x == NULL) {                    \
    IOTC_LOG(F(TO_STR(x) "is NULL")); \
    return 1;                         \
  }

#define GET_LENGTH_NOT_NULL_NOT_EMPTY(x, maxlen)                 \
  unsigned x##_len = 0;                                          \
  do {                                                           \
    CHECK_NOT_NULL(x)                                            \
    x##_len = strlen_s_(x, INT_MAX);                             \
    if (x##_len == 0 || x##_len > maxlen) {                      \
      IOTC_LOG(F("ERROR: " TO_STR(x) "has length %d"), x##_len); \
      return 1;                                                  \
    }                                                            \
  } while (0)

#define GET_LENGTH_NOT_NULL(x, maxlen)                            \
  unsigned x##_len = 0;                                           \
  do {                                                            \
    CHECK_NOT_NULL(x)                                             \
    x##_len = strlen_s_(x, INT_MAX);                              \
    if (x##_len > maxlen) {                                       \
      IOTC_LOG(F("ERROR: " TO_STR(x) " has length %d"), x##_len); \
      return 1;                                                   \
    }                                                             \
  } while (0)

#define MUST_CALL_BEFORE_INIT(x)                                      \
  if (x != NULL) {                                                    \
    IOTC_LOG(F("ERROR: Client was already initialized. ERR:0x0006")); \
    return 6;                                                         \
  }

#define MUST_CALL_AFTER_INIT(x)                                   \
  if (x == NULL) {                                                \
    IOTC_LOG(F("ERROR: Client was not initialized. ERR:0x0007")); \
    return 7;                                                     \
  }

#ifdef USE_LIGHT_CLIENT
#define MUST_CALL_AFTER_CONNECT(x)                   \
  if (x == NULL || x->mqttClient == NULL) {          \
    IOTC_LOG(F("ERROR: Client was not connected.")); \
    return 1;                                        \
  }
#else  // USE_LIGHT_CLIENT
#define MUST_CALL_AFTER_CONNECT(x)                   \
  if (x == NULL || x->clientHandle == NULL) {        \
    IOTC_LOG(F("ERROR: Client was not connected.")); \
    return 1;                                        \
  }
#endif  // USE_LIGHT_CLIENT

#define HOSTNAME_STRING "HostName="
#define DEVICEID_STRING ";DeviceId="
#define KEY_STRING ";SharedAccessKey="
#define HOSTNAME_LENGTH (sizeof(HOSTNAME_STRING) - 1)
#define DEVICEID_LENGTH (sizeof(DEVICEID_STRING) - 1)
#define KEY_LENGTH (sizeof(KEY_STRING) - 1)

#ifdef __cplusplus
extern "C" {
#endif

unsigned strlen_s_(const char *str, int max_expected);
int getUsernameAndPasswordFromConnectionString(
    const char *connectionString, size_t connectionStringLength,
    AzureIOT::StringBuffer &hostName, AzureIOT::StringBuffer &deviceId,
    AzureIOT::StringBuffer &username, AzureIOT::StringBuffer &password);

int getDPSAuthString(const char *scopeId, const char *deviceId, const char *key,
                     char *buffer, int bufferSize, size_t &outLength);

void setLogLevel(IOTLogLevel l);
IOTLogLevel getLogLevel();

void handlePayload(char *msg, unsigned long msg_length, char *topic,
                   unsigned long topic_length);
int getHubHostName(IOTContextInternal* internal, const char *dpsEndpoint, const char *scopeId,
                   const char *deviceId, const char *key, char *hostName);
void connectionStatusCallback(IOTConnectionState status,
                              IOTContextInternal *internal);
IOTContextInternal *getSingletonContext();
void setSingletonContext(IOTContextInternal *ctx);
void sendConfirmationCallback(const char *buffer, size_t size);

int mqtt_publish(IOTContextInternal *internal, const char *topic,
                 unsigned long topic_length, const char *msg,
                 unsigned long msg_length);

#ifdef __cplusplus
}
#endif

#ifdef ARDUINO
#define IOTC_LOG(...)                          \
  if (getLogLevel() > IOTC_LOGGING_DISABLED) { \
    Serial.printf("  - ");                     \
    Serial.printf(__VA_ARGS__);                \
    Serial.printf("\r\n");                     \
  }
#undef F
#define F(x) x
#endif

#endif  // AZURE_IOTC_INTERNAL_H