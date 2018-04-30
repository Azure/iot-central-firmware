// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"


#include "LSM6DSLSensor.h"
#include "LIS2MDLSensor.h"
#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "RGB_LED.h"
#include "IrDASensor.h"

#include "../inc/sensors.h"

DevI2C *i2c;
LSM6DSLSensor *accelGyro;
LIS2MDLSensor *magnetometer;
HTS221Sensor *tempHumidity;
LPS22HBSensor *pressure;
RGB_LED rgbLed;
IRDASensor *irdaSensor;

void initSensors() {
    // LSM6DSL
    i2c = new DevI2C(D14, D15);
    accelGyro = new LSM6DSLSensor(*i2c, D4, D5);
    accelGyro->init(NULL);
    accelGyro->enableAccelerator();
    accelGyro->enableGyroscope();

    // enable double tap detection
    accelGyro->enablePedometer();
    accelGyro->setPedometerThreshold(LSM6DSL_PEDOMETER_THRESHOLD_MID_LOW);

    // LIS2MDL
    magnetometer = new LIS2MDLSensor(*i2c);
    magnetometer->init(NULL);

    // HTS221
    tempHumidity = new HTS221Sensor(*i2c);
    tempHumidity->init(NULL);

    // LPS22HB
    pressure = new LPS22HBSensor(*i2c);
    pressure->init(NULL);


    // IrDA
    irdaSensor = new IRDASensor();
    irdaSensor->init();
}

// HTS221
float readHumidity() {
    float humidityValue;
    tempHumidity->reset();
    if (tempHumidity->getHumidity(&humidityValue) == 0)
        return humidityValue;
    else
        return 0xFFFF;
}

float readTemperature() {
    float tempValue;
    tempHumidity->reset();
    if (tempHumidity->getTemperature(&tempValue) == 0)
        return tempValue;
    else
        return 0xFFFF;
}

// LPS22HB
float readPressure() {
    float presureValue;
    if (pressure->getPressure(&presureValue) == 0)
        return presureValue;
    else
        return 0xFFFF;
}

// LIS2MDL
void readMagnetometer(int *axes) {
    if (magnetometer->getMAxes(axes) != 0) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

// LSM6DSL
void readAccelerometer(int *axes) {
    if (accelGyro->getXAxes(axes) != 0) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

void readGyroscope(int *axes) {
    if (accelGyro->getGAxes(axes) != 0) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

bool checkForShake() {
    int steps = 0;
    bool shake = false;
    accelGyro->getStepCounter(&steps);
    if (steps > 1) {
        shake = true;
    }
    accelGyro->resetStepCounter();

    return shake;
}

// RGB LED
void setLedColor(uint8_t red, uint8_t green, uint8_t blue) {
    rgbLed.setColor(red, green, blue);
}

void turnLedOff() {
    rgbLed.turnOff();
}

void transmitIR() {
    //static unsigned char data[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

    unsigned char data = 1;
    for (int i = 0; i < 20; i++) {
        int irda_status = irdaSensor->IRDATransmit(&data, 1, 100);
        if(irda_status != 0)
        {
            LOG_ERROR("Unable to transmit through IrDA");
        }
        delay(150);
    }
}