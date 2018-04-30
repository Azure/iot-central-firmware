// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef GLOBALS_H
#define GLOBALS_H

#include <assert.h>
#include <stdint.h>
#include <Arduino.h>
#include <limits.h>

// GENERAL
#define TO_STRING_(s) #s
#define TO_STRING(s) TO_STRING_(s)

#define STRING_BUFFER_16     16
#define STRING_BUFFER_32     32
#define STRING_BUFFER_128   128
#define STRING_BUFFER_512   512
#define STRING_BUFFER_1024 1024
#define STRING_BUFFER_4096 4096

typedef enum { NORMAL, CAUTION, DANGER } DeviceState;

class AzWebServer;
class IoTHubClient;
class WiFiController;
class SensorController;

struct Globals
{
    static  bool              isConfigured; // default: false
    static  bool              needsInitialize; // default: true
    static  AzWebServer       webServer;
    static  const char *      completedString; // \"completed\"
    static  IoTHubClient *    iothubClient;
    static  WiFiController    wiFiController;
    static  SensorController  sensorController;
};

// IOT HUB
#define MAX_CALLBACK_COUNT 32

// DEVICE SPECIFIC
#define AZ3166_DISPLAY_MAX_COLUMN 16

// IOT CENRAL SPECIFIC
#define IOT_CENTRAL_ZONE_IDX      0x02
#define IOT_CENTRAL_MAX_LEN       STRING_BUFFER_128
#define AZIOTC_FW_MAJOR_VERSION 1
#define AZIOTC_FW_MINOR_VERSION 0
#define AZIOTC_FW_PATCH_VERSION 0
#define AZIOTC_FW_VERSION         TO_STRING(AZIOTC_FW_MAJOR_VERSION AZIOTC_FW_MINOR_VERSION AZIOTC_FW_PATCH_VERSION) "-MSIOTC"

// LOGS

#define SERIAL_VERBOSE_LOGGING_ENABLED false

#if SERIAL_VERBOSE_LOGGING_ENABLED != 1
#define LOG_VERBOSE(...)
#else
#define LOG_VERBOSE(...) \
    do { \
        Serial.printf(__VA_ARGS__); \
        Serial.printf('\r\n'); \
    } while(0)
#endif

// Log Errors no matter what
#define LOG_ERROR(...) \
    do { \
        Serial.printf("Error at %s:%d\r\n\t", __FILE__, __LINE__); \
        Serial.printf(__VA_ARGS__); \
        Serial.printf("\r\n"); \
    } while(0)

#endif // GLOBALS_H