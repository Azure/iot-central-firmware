// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

// src/connection.h

bool isConnected = false;
unsigned long lastTick = 0, loopId = 0;
IOTContext context = NULL;

void connect_client(const char* scopeId, const char* deviceId,
                    const char* deviceKey) {
  // initialize iotc context (per device client)
  int errorCode = iotc_init_context(&context);
  if (errorCode != 0) {
    LOG_ERROR("Error initializing IOTC. Code %d", errorCode);
    return;
  }

  iotc_set_logging(IOTC_LOGGING_API_ONLY);

  // set up event callbacks. they are all declared under the ESP8266.ino file
  // for simplicity, track all of them from the same callback function
  iotc_on(context, "MessageSent", on_event, NULL);
  iotc_on(context, "Command", on_event, NULL);
  iotc_on(context, "ConnectionStatus", on_event, NULL);
  iotc_on(context, "SettingsUpdated", on_event, NULL);

  // connect to Azure IoT
  errorCode = iotc_connect(context, scopeId, deviceKey, deviceId,
                           IOTC_CONNECT_SYMM_KEY);
  if (errorCode != 0) {
    LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
    return;
  }
}

void connect_wifi(const char* wifi_ssid, const char* wifi_password) {
  WiFi.begin(wifi_ssid, wifi_password);

  LOG_VERBOSE("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}
