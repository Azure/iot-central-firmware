// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef CONFIG_H
#define CONFIG_H

void clearAllConfig();

void storeWiFi(const char *ssid, const char *password);
void storeConnectionString(const char *connectionString);
void storeIotCentralConfig(const char *iotCentralConfig, int size);

void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen);
String readConnectionString();
String readIotCentralConfig();

void clearWiFiEEPROM();
void clearAzureEEPROM();
void clearIotCentralEEPROM();

#endif /* CONFIG_H */

#define TEMP_CHECKED 0x80
#define HUMIDITY_CHECKED 0x40
#define PRESSURE_CHECKED 0x20
#define ACCEL_CHECKED 0x10
#define GYRO_CHECKED 0x08
#define MAG_CHECKED 0x04
