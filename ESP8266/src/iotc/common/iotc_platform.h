// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef IOTC_COMMON_PLATFORM_H
#define IOTC_COMMON_PLATFORM_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#define ARDUINO_WIFI_SSL_CLIENT WiFiClientSecure
#define USE_LIGHT_CLIENT 1

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "../arduino/PubSubClient.h"
#include "WiFiUdp.h"
#include "base64.h"
#include "sha256.h"
#define WAITMS delay
#define SERIAL_PRINT Serial.print

#ifndef PROGMEM
#define PROGMEM
#endif

#endif  // IOTC_COMMON_PLATFORM_H
