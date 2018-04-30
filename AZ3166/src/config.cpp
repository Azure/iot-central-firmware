// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include <EEPROMInterface.h>

#include "../inc/utility.h"
#include "../inc/config.h"

void clearAllConfig() {
    clearWiFiEEPROM();
    clearAzureEEPROM();
    clearIotCentralEEPROM();
}

void storeWiFi(AutoString &ssid, AutoString &password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*) *ssid, ssid.getLength(), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*) *password, password.getLength(), WIFI_PWD_ZONE_IDX);
}

void storeConnectionString(AutoString &connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) *connectionString, connectionString.getLength(), AZ_IOT_HUB_ZONE_IDX);
}

void storeIotCentralConfig(AutoString &iotCentralConfig) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) *iotCentralConfig, iotCentralConfig.getLength(), IOT_CENTRAL_ZONE_IDX);
}

void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen) {
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*) ssid, ssidLen, 0, WIFI_SSID_ZONE_IDX);
    eeprom.read((uint8_t*) password, passwordLen, 0, WIFI_PWD_ZONE_IDX);
}

void readConnectionString(char* connectionString, uint32_t buffer_size) {
    EEPROMInterface eeprom;
    assert(connectionString != NULL);
    assert(buffer_size == AZ_IOT_HUB_MAX_LEN);

    eeprom.read((uint8_t*) connectionString, AZ_IOT_HUB_MAX_LEN, 0, AZ_IOT_HUB_ZONE_IDX);
}

void readIotCentralConfig(char* iotCentralConfig, uint32_t buffer_size) {
    EEPROMInterface eeprom;
    assert(iotCentralConfig != NULL);
    assert(buffer_size == IOT_CENTRAL_MAX_LEN);

    eeprom.read((uint8_t*) iotCentralConfig, IOT_CENTRAL_MAX_LEN, 0, IOT_CENTRAL_ZONE_IDX);
}

void clearWiFiEEPROM() {
    EEPROMInterface eeprom;

    uint32_t max_mem = max(WIFI_SSID_MAX_LEN, WIFI_PWD_MAX_LEN);
    AutoString cleanBuffer(max_mem);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_PWD_MAX_LEN, WIFI_PWD_ZONE_IDX);
}

void clearAzureEEPROM() {
    EEPROMInterface eeprom;

    AutoString cleanBuffer(AZ_IOT_HUB_MAX_LEN);
    eeprom.write((uint8_t*)*cleanBuffer, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
}

void clearIotCentralEEPROM() {
    EEPROMInterface eeprom;

    AutoString cleanBuffer(IOT_CENTRAL_MAX_LEN);
    eeprom.write((uint8_t*)*cleanBuffer, IOT_CENTRAL_MAX_LEN, IOT_CENTRAL_ZONE_IDX);
}