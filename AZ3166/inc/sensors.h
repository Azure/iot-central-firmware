// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef SENSORS_H
#define SENSORS_H

void initSensors();

// HTS221
float readHumidity();
float readTemperature();

// LPS22HB
float readPressure();

// LIS2MDL
void readMagnetometer(int *axes);

// LSM6DSL
void readAccelerometer(int *axes);
void readGyroscope(int *axes);
bool checkForShake();

// RGB LED
void setLedColor(uint8_t red, uint8_t green, uint8_t blue);
void turnLedOff();

// IrDA
void transmitIR();

#endif /* SENSORS_H */