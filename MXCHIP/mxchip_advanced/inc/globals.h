// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef GLOBALS_H
#define GLOBALS_H

#include <assert.h>
#include <stdint.h>
#include <string>
#include <map>
#include <Arduino.h>
#include <limits.h>

// GENERAL
#define TO_STRING_(s) #s
#define TO_STRING(s) TO_STRING_(s)

#define HUMIDITY_CHECKED 0x01
#define MAG_CHECKED 0x02
#define TEMP_CHECKED 0x04
#define PRESSURE_CHECKED 0x08
#define ACCEL_CHECKED 0x10
#define GYRO_CHECKED 0x20

#define STRING_BUFFER_16     16
#define STRING_BUFFER_32     32
#define STRING_BUFFER_128   128
#define STRING_BUFFER_256   256
#define STRING_BUFFER_512   512
#define STRING_BUFFER_1024 1024
#define STRING_BUFFER_4096 4096

#define OLED_SINGLE_FRAME_BUFFER 576
#define SAS_SCOPE_ID_ENDS 64
#define SAS_REG_ID_ENDS 192

#define TELEMETRY_SEND_INTERVAL 5000
#define TELEMETRY_REPORTED_SEND_INTERVAL 2000
#define TELEMETRY_SWITCH_DEBOUNCE_TIME 250

#define NTP_SYNC_PERIOD (24 * 60 * 60 * 1000)

typedef enum { NORMAL, CAUTION, DANGER } DeviceState;

#define STATE_MESSAGE(state) \
    state == NORMAL ?  "{\"deviceState\":\"NORMAL\"}" :  ( \
    state == CAUTION ? "{\"deviceState\":\"CAUTION\"}" : ( \
    state == DANGER ?  "{\"deviceState\":\"DANGER\"}" :  ( \
                       "{\"deviceState\":\"NORMAL\"}"      \
    ) \
    ) \
    )

class WiFiController;
class SensorController;
class LoopController;

#define MAP_DATA_SIZE 546

#define EMPTY_JSON "{}"
struct Globals
{
    static  WiFiController          wiFiController;
    static  SensorController        sensorController;
    static  LoopController *        loopController;

    static double                   locationData[MAP_DATA_SIZE];
};

// DEVICE SPECIFIC
#define AZ3166_DISPLAY_MAX_COLUMN 16

// IOT CENRAL SPECIFIC
#define IOT_CENTRAL_ZONE_IDX      0x02
#define IOT_CENTRAL_MAX_LEN       STRING_BUFFER_128
#define AZIOTC_FW_MAJOR_VERSION 2
#define AZIOTC_FW_MINOR_VERSION 1
#define AZIOTC_FW_PATCH_VERSION 1
#define AZIOTC_FW_VERSION         TO_STRING(AZIOTC_FW_MAJOR_VERSION AZIOTC_FW_MINOR_VERSION AZIOTC_FW_PATCH_VERSION) "-MSIOTC"

#include "definitions.h"

// LOGS

#define IOTHUB_TRACE_LOG_ENABLED       false

// To enable Fan Sound experiment, comment out the line below
// CAUTION: Fan sound is aprox 80KB . So, make sure you got enough sram left ;)
// #define ENABLE_FAN_SOUND

#include "../src/iotc/iotc.h"

#endif // GLOBALS_H