// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOTC_API
#define AZURE_IOTC_API

#ifdef TARGET_MXCHIP
#include <Arduino.h>
#endif

#define AZIOTC_MAJOR_VERSION 0
#define AZIOTC_MINOR_VERSION 1
#define AZIOTC_PATCH_VERSION 0
#define AZIOTC_VERSION       \
  TO_STRING(AZIOTC_MAJOR_VERSION) "." TO_STRING(AZIOTC_MINOR_VERSION) "." TO_STRING(AZIOTC_PATCH_VERSION)

#ifdef __cplusplus
extern "C" {
#endif

// ***** Type definitions *****
typedef struct IOTC_HTTP_PROXY_OPTIONS_TAG
{
  const char* host_address;
  int port;
  const char* username;
  const char* password;
} IOTC_HTTP_PROXY_OPTIONS;

typedef struct IOTCallbackInfo_TAG {
  const char* eventName;
  const char* tag;
  const char* payload;
  unsigned    payloadLength;

  void *appContext;

  int statusCode;
  void *callbackResponse;
} IOTCallbackInfo;

typedef void* IOTContext;

// ***** Macro definitions *****
#define IOTC_PROTOCOL_MQTT 0x01
#define IOTC_PROTOCOL_AMQP 0x02
#define IOTC_PROTOCOL_HTTP 0x04
typedef short IOTProtocol;

#define IOTC_LOGGING_DISABLED 0x01
#define IOTC_LOGGING_API_ONLY 0x02
#define IOTC_LOGGING_ALL      0x10
typedef short IOTLogLevel;

#define IOTC_CONNECT_SYMM_KEY          0x01
#define IOTC_CONNECT_X509_CERT         0x02
#define IOTC_CONNECT_CONNECTION_STRING 0x04
typedef short IOTConnectType;

#define IOTC_CONNECTION_EXPIRED_SAS_TOKEN    0x01
#define IOTC_CONNECTION_DEVICE_DISABLED      0x02
#define IOTC_CONNECTION_BAD_CREDENTIAL       0x04
#define IOTC_CONNECTION_RETRY_EXPIRED        0x08
#define IOTC_CONNECTION_NO_NETWORK           0x10
#define IOTC_CONNECTION_COMMUNICATION_ERROR  0x20
#define IOTC_CONNECTION_OK                   0x40
#define IOTC_CONNECTION_DISCONNECTED         0x80
typedef short IOTConnectionState;

#define IOTC_MESSAGE_ACCEPTED   0x01
#define IOTC_MESSAGE_REJECTED   0x02
#define IOTC_MESSAGE_ABANDONED  0x04
typedef short IOTMessageStatus;

// ***** API *****
// Set the level of logging (see the options above)
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_logging(IOTLogLevel level);

// Initialize the device context. The context variable will be used by rest of the API
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_init_context(IOTContext *ctx);

// Free device context.
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_free_context(IOTContext ctx);

// Connect to Azure IoT Central
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
                                 const char* deviceId, IOTConnectType type);

// Disconnect
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_disconnect(IOTContext ctx);

// If your endpoint is different than the default AzureIoTCentral endpoint, set it
// using this API.
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_global_endpoint(IOTContext ctx, const char* endpoint_uri);

// Set the custom certificates for custom endpoints
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_trusted_certs(IOTContext ctx, const char* certs);

// Set the proxy settings
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_proxy(IOTContext ctx, IOTC_HTTP_PROXY_OPTIONS proxy);

// Sends a telemetry payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_telemetry(IOTContext ctx, const char* payload, unsigned length);

// Sends a state payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_state    (IOTContext ctx, const char* payload, unsigned length);

// Sends an event payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_event    (IOTContext ctx, const char* payload, unsigned length);

// Sends a property payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_property (IOTContext ctx, const char* payload, unsigned length);

/*
eventName:
  ConnectionStatus
  MessageSent
  Command
  SettingsUpdated
  Error
*/
typedef void(*IOTCallback)(IOTContext, IOTCallbackInfo*);

// Register to one of the events listed above
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_on(IOTContext ctx, const char* eventName, IOTCallback callback, void* appContext);

// Lets SDK to do background work
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_do_work(IOTContext ctx);

// Provide platform dependent NetworkInterface
int iotc_set_network_interface(void* networkInterface);

#ifdef ARDUINO
    #define SERIAL_PRINT Serial.printf
#else
    #define SERIAL_PRINT printf
#endif

#define SERIAL_VERBOSE_LOGGING_ENABLED 1

#ifndef LOG_VERBOSE
#if SERIAL_VERBOSE_LOGGING_ENABLED != 1
#define LOG_VERBOSE(...)
#else
#define LOG_VERBOSE(...) \
    do { \
        SERIAL_PRINT("  - "); \
        SERIAL_PRINT(__VA_ARGS__); \
        SERIAL_PRINT("\r\n"); \
    } while(0)
#endif // SERIAL_VERBOSE_LOGGING_ENABLED != 1

// Log Errors no matter what
#define LOG_ERROR(...) \
    do { \
        SERIAL_PRINT("X - Error at %s:%d\r\n\t", __FILE__, __LINE__); \
        SERIAL_PRINT(__VA_ARGS__); \
        SERIAL_PRINT("\r\n"); \
    } while(0)
#endif // !LOG_VERBOSE

#ifdef __cplusplus
}
#endif

#endif // AZURE_IOTC_API