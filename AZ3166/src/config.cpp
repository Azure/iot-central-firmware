// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"
#include "EEPROMInterface.h"

#include "../inc/iotCentral.h"
#include "../inc/config.h"

void clearAllConfig() {
    clearWiFiEEPROM();
    clearAzureEEPROM();
    clearIotCentralEEPROM();
}

void storeWiFi(const char *ssid, const char *password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*)ssid, strlen(ssid), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*)password, strlen(password), WIFI_PWD_ZONE_IDX);
}

void storeConnectionString(const char *connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*)connectionString, strlen(connectionString), AZ_IOT_HUB_ZONE_IDX);
}

void storeIotCentralConfig(const char *iotCentralConfig, int size) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*)iotCentralConfig, size, IOT_CENTRAL_ZONE_IDX);
}

void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen) {
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*)ssid, ssidLen, 0, WIFI_SSID_ZONE_IDX);
    eeprom.read((uint8_t*)password, passwordLen, 0, WIFI_PWD_ZONE_IDX);
}

String readConnectionString() {
    EEPROMInterface eeprom;
    uint8_t connectionString[AZ_IOT_HUB_MAX_LEN];
    eeprom.read((uint8_t*)connectionString, AZ_IOT_HUB_MAX_LEN, 0, AZ_IOT_HUB_ZONE_IDX);
    return String((const char*)connectionString);
}

String readIotCentralConfig() {
    EEPROMInterface eeprom;
    uint8_t iotCentralConfig[IOT_CENTRAL_MAX_LEN];
    eeprom.read(iotCentralConfig, IOT_CENTRAL_MAX_LEN, 0, IOT_CENTRAL_ZONE_IDX);
    return String((const char*)iotCentralConfig);
}

void clearWiFiEEPROM() {
    EEPROMInterface eeprom;
    
    uint8_t *cleanBuff = (uint8_t*) malloc(WIFI_SSID_MAX_LEN);
    memset(cleanBuff, 0x00, WIFI_SSID_MAX_LEN);
    eeprom.write(cleanBuff, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    free(cleanBuff);

    cleanBuff = (uint8_t*) malloc(WIFI_PWD_MAX_LEN);
    memset(cleanBuff, 0x00, WIFI_PWD_MAX_LEN);
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
    
    uint8_t *cleanBuff = (uint8_t*) malloc(IOT_CENTRAL_MAX_LEN);
    memset(cleanBuff, 0x00, IOT_CENTRAL_MAX_LEN);
    eeprom.write(cleanBuff, IOT_CENTRAL_MAX_LEN, IOT_CENTRAL_ZONE_IDX);
    free(cleanBuff);
}