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

void ConfigController::storeWiFi(StringBuffer &ssid, StringBuffer &password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*) *ssid, ssid.getLength(), WIFI_SSID_ZONE_IDX);
    if (password.getLength()) {
        eeprom.write((uint8_t*) *password, password.getLength(), WIFI_PWD_ZONE_IDX);
    } else {
        eeprom.write((uint8_t*) "", 0, WIFI_PWD_ZONE_IDX);
    }
}

void ConfigController::storeConnectionString(StringBuffer &connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) *connectionString, connectionString.getLength(), AZ_IOT_HUB_ZONE_IDX);
}

void ConfigController::storeIotCentralConfig(StringBuffer &iotCentralConfig) {
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
        // force null ending and re populate
        char ssid_buffer[STRING_BUFFER_1024] = {0};
        char pass_buffer[STRING_BUFFER_1024] = {0};
        strcpy(ssid_buffer, WIFI_NAME);
        strcpy(pass_buffer, WIFI_PASSWORD);
        eeprom.write((uint8_t*) ssid_buffer, sizeof(WIFI_NAME), WIFI_SSID_ZONE_IDX);
        eeprom.write((uint8_t*) pass_buffer, sizeof(WIFI_PASSWORD), WIFI_PWD_ZONE_IDX);
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

void ConfigController::storeKey(StringBuffer &auth, StringBuffer &scopeId, StringBuffer &regId, StringBuffer &key) {
    char buffer[AZ_IOT_HUB_MAX_LEN] = {0};
    memcpy(buffer, *scopeId, scopeId.getLength());
    memcpy(buffer + SAS_SCOPE_ID_ENDS, *regId, regId.getLength());
    if (key.getLength() > 0) {
        // 0 length for x509 sample cert
        memcpy(buffer + SAS_REG_ID_ENDS, *key, key.getLength());
    }
    LOG_VERBOSE("STORE scope:%s regId:%s key:%s", scopeId.getLength() > 0 ? *scopeId : "", *regId, *key);
    if ((*auth)[0] == 'S') {
        LOG_VERBOSE("\tKEY SSYM");
        memcpy(buffer + (AZ_IOT_HUB_MAX_LEN - 4), "SSYM", 4);
    } else if ((*auth)[0] == 'C') {
        LOG_VERBOSE("\tKEY Connection String");
        memcpy(buffer + (AZ_IOT_HUB_MAX_LEN - 4), "CSTR", 4);
        memcpy(buffer, *key, key.getLength());
        buffer[key.getLength()] = 0;
    } else {
        LOG_VERBOSE("\tKEY X509");
        memcpy(buffer + (AZ_IOT_HUB_MAX_LEN - 4), "X509", 4);
    }

    EEPROMInterface eeprom;
    eeprom.write((uint8_t*) buffer, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
}

void ConfigController::readGroupSXKeyAndDeviceId(char * scopeId, char * registrationId, char * sas, char &atype) {
#if !defined(IOT_CENTRAL_SAS_KEY) && !defined(IOT_CENTRAL_USE_X509_SAMPLE_CERT)
    char buffer[AZ_IOT_HUB_MAX_LEN];
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*) buffer, AZ_IOT_HUB_MAX_LEN, 0, AZ_IOT_HUB_ZONE_IDX);
    atype = strncmp(buffer + (AZ_IOT_HUB_MAX_LEN - 4), "SSYM", 4) == 0 ? 'S' : ' ';

    if (atype == ' ' && strncmp(buffer + (AZ_IOT_HUB_MAX_LEN - 4), "X509", 4) == 0)
        atype = 'X';
    else if (atype == ' ')
        atype = 'C';

    if (atype == 'C') {
        strcpy(sas, buffer);
        *scopeId = '\0';
        *registrationId = '\0';
    } else {
        strcpy(scopeId, buffer);
        strcpy(registrationId, buffer + SAS_SCOPE_ID_ENDS);
        strcpy(sas, buffer + SAS_REG_ID_ENDS);
    }
#else // !defined(IOT_CENTRAL_SAS_KEY) && !defined(IOT_CENTRAL_USE_X509_SAMPLE_CERT)
#if defined(IOT_CENTRAL_SAS_KEY)
    strcpy(sas, IOT_CENTRAL_SAS_KEY);
    strcpy(registrationId, IOT_CENTRAL_REGISTRATION_ID);
    atype = 'S';
#else // defined(IOT_CENTRAL_SAS_KEY)
    sas[0] = 0;
    atype = 'X';
    strcpy(registrationId, "riot-device-cert");
#endif // defined(IOT_CENTRAL_SAS_KEY)
    strcpy(scopeId, IOT_CENTRAL_SCOPE_ID);
#endif // !defined(IOT_CENTRAL_SAS_KEY) && !defined(IOT_CENTRAL_USE_X509_SAMPLE_CERT)
    assert(atype == 'X' || atype == 'C' || atype == 'S');
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
    LOG_VERBOSE("ConfigController::clearWiFiEEPROM");
    EEPROMInterface eeprom;

    uint32_t max_mem = max(WIFI_SSID_MAX_LEN, WIFI_PWD_MAX_LEN);
    StringBuffer cleanBuffer(max_mem);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*)*cleanBuffer, WIFI_PWD_MAX_LEN, WIFI_PWD_ZONE_IDX);
}

void ConfigController::clearAzureEEPROM() {
    LOG_VERBOSE("ConfigController::clearAzureEEPROM");
    EEPROMInterface eeprom;

    char cleanBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    eeprom.write((uint8_t*)cleanBuffer, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
}

void ConfigController::clearIotCentralEEPROM() {
    LOG_VERBOSE("ConfigController::clearIotCentralEEPROM");
    EEPROMInterface eeprom;

    char cleanBuffer[AZ_IOT_HUB_MAX_LEN] = {0};
    eeprom.write((uint8_t*)cleanBuffer, IOT_CENTRAL_MAX_LEN, IOT_CENTRAL_ZONE_IDX);
}