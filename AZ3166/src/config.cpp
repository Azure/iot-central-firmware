// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include <EEPROMInterface.h>

#include "../inc/config.h"

void clearAllConfig() {
    clearWiFiEEPROM();
    clearAzureEEPROM();
    clearIotCentralEEPROM();
}

void storeWiFi(const char *ssid, const char *password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*) ssid, strlen(ssid), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*) password, strlen(password), WIFI_PWD_ZONE_IDX);
}

void storeConnectionString(const char *connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) connectionString, strlen(connectionString), AZ_IOT_HUB_ZONE_IDX);
}

void storeIotCentralConfig(const char *iotCentralConfig, int size) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) iotCentralConfig, size, IOT_CENTRAL_ZONE_IDX);
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
    uint8_t *cleanBuff = (uint8_t*) malloc(max_mem);
    memset(cleanBuff, 0x00, max_mem);
    eeprom.write(cleanBuff, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    eeprom.write(cleanBuff, WIFI_PWD_MAX_LEN, WIFI_PWD_ZONE_IDX);
    free(cleanBuff);
}

void clearAzureEEPROM() {
    EEPROMInterface eeprom;

    uint8_t *cleanBuff = (uint8_t*) malloc(AZ_IOT_HUB_MAX_LEN);
    memset(cleanBuff, 0x00, AZ_IOT_HUB_MAX_LEN);
    eeprom.write(cleanBuff, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
    free(cleanBuff);
}

void clearIotCentralEEPROM() {
    EEPROMInterface eeprom;

    uint8_t *cleanBuff = (uint8_t*) malloc(IOT_CENTRAL_MAX_LEN * sizeof(uint8_t));
    memset(cleanBuff, 0x00, IOT_CENTRAL_MAX_LEN);
    eeprom.write(cleanBuff, IOT_CENTRAL_MAX_LEN, IOT_CENTRAL_ZONE_IDX);
    free(cleanBuff);
}