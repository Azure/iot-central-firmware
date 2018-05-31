// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include <EEPROMInterface.h>

#include "../inc/utility.h"
#include "../inc/config.h"

void ConfigController::clearAllConfig() {
    ConfigController::clearWiFiEEPROM();
    ConfigController::clearAzureEEPROM();
    ConfigController::clearIotCentralEEPROM();
}

void ConfigController::storeWiFi(AutoString &ssid, AutoString &password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*) *ssid, ssid.getLength(), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*) *password, password.getLength(), WIFI_PWD_ZONE_IDX);
}

void ConfigController::storeConnectionString(AutoString &connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) *connectionString, connectionString.getLength(), AZ_IOT_HUB_ZONE_IDX);
}

void ConfigController::storeIotCentralConfig(AutoString &iotCentralConfig) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) *iotCentralConfig, iotCentralConfig.getLength(), IOT_CENTRAL_ZONE_IDX);
}

// Unused. Left here for sample
void ConfigController::readWiFi(char* ssid, int ssidLen, char *password, int passwordLen) {
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*) ssid, ssidLen, 0, WIFI_SSID_ZONE_IDX);
    eeprom.read((uint8_t*) password, passwordLen, 0, WIFI_PWD_ZONE_IDX);
#if defined(WIFI_NAME)
    // check if hardcoded wifi creds are equal to the ones in eeprom
    if (strcmp(ssid, WIFI_NAME) != 0 || strcmp(password, WIFI_PASSWORD) != 0) {
        eeprom.write((uint8_t*) WIFI_NAME, strlen(WIFI_NAME), WIFI_SSID_ZONE_IDX);
        eeprom.write((uint8_t*) WIFI_PASSWORD, strlen(WIFI_PASSWORD), WIFI_PWD_ZONE_IDX);
    }
#endif
}

void ConfigController::readConnectionString(char* connectionString, uint32_t buffer_size) {
#if !defined(IOT_CENTRAL_CONNECTION_STRING)
    EEPROMInterface eeprom;
    assert(connectionString != NULL);
    assert(buffer_size == AZ_IOT_HUB_MAX_LEN);

    eeprom.read((uint8_t*) connectionString, AZ_IOT_HUB_MAX_LEN, 0, AZ_IOT_HUB_ZONE_IDX);
#else // !defined(IOT_CENTRAL_CONNECTION_STRING)
    assert(sizeof(IOT_CENTRAL_CONNECTION_STRING) - 1 < buffer_size);
    strcpy(connectionString, IOT_CENTRAL_CONNECTION_STRING);
#endif // !defined(IOT_CENTRAL_CONNECTION_STRING)
}

void ConfigController::readIotCentralConfig(char* iotCentralConfig, uint32_t buffer_size) {
    assert(iotCentralConfig != NULL);
    assert(buffer_size == IOT_CENTRAL_MAX_LEN);

#ifdef COMPILE_TIME_DEFINITIONS_SET
    uint8_t checkboxState = 0x00; // bit order - see globals.h
    #ifndef DISABLE_TEMPERATURE
        checkboxState |= TEMP_CHECKED;
    #endif
    #ifndef DISABLE_PRESSURE
        checkboxState |= PRESSURE_CHECKED;
    #endif
    #ifndef DISABLE_GYROSCOPE
        checkboxState |= GYRO_CHECKED;
    #endif
    #ifndef DISABLE_ACCELEROMETER
        checkboxState |= ACCEL_CHECKED;
    #endif
    #ifndef DISABLE_HUMIDITY
        checkboxState |= HUMIDITY_CHECKED;
    #endif
    #ifndef DISABLE_MAGNETOMETER
        checkboxState |= MAG_CHECKED;
    #endif
    snprintf(iotCentralConfig, 3, "%d", checkboxState);

    char ssid[STRING_BUFFER_1024], pass[STRING_BUFFER_1024];
    readWiFi(ssid, 1024, pass, 1024); // to force resetting wifi creds into eeprom
#else // COMPILE_TIME_DEFINITIONS_SET
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*) iotCentralConfig, IOT_CENTRAL_MAX_LEN, 0, IOT_CENTRAL_ZONE_IDX);
#endif // COMPILE_TIME_DEFINITIONS_SET
}

void ConfigController::clearWiFiEEPROM() {
    EEPROMInterface eeprom;

    uint32_t max_mem = max(WIFI_SSID_MAX_LEN, WIFI_PWD_MAX_LEN);
    AutoString cleanBuffer(max_mem);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_PWD_MAX_LEN, WIFI_PWD_ZONE_IDX);
}

void ConfigController::clearAzureEEPROM() {
    EEPROMInterface eeprom;

    AutoString cleanBuffer(AZ_IOT_HUB_MAX_LEN);
    eeprom.write((uint8_t*)*cleanBuffer, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
}

void ConfigController::clearIotCentralEEPROM() {
    EEPROMInterface eeprom;

    AutoString cleanBuffer(IOT_CENTRAL_MAX_LEN);
    eeprom.write((uint8_t*)*cleanBuffer, IOT_CENTRAL_MAX_LEN, IOT_CENTRAL_ZONE_IDX);
}