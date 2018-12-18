// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef SENSORS_H
#define SENSORS_H

class DevI2C;
class LSM6DSLSensor;
class LIS2MDLSensor;
class HTS221Sensor;
class LPS22HBSensor;
class IRDASensor;

#include "RGB_LED.h"

class SensorController
{
    DevI2C *i2c;
    LSM6DSLSensor *accelGyro;
    LIS2MDLSensor *magnetometer;
    HTS221Sensor *tempHumidity;
    LPS22HBSensor *pressure;
    RGB_LED rgbLed;
    IRDASensor *irdaSensor;
public:
    SensorController(): i2c(NULL), accelGyro(NULL),
                        magnetometer(NULL), tempHumidity(NULL), pressure(NULL),
                        irdaSensor(NULL) { }

    ~SensorController();

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
};

#endif /* SENSORS_H */