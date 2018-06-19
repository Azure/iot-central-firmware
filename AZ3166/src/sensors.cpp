// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"


#include "LSM6DSLSensor.h"
#include "LIS2MDLSensor.h"
#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "IrDASensor.h"

#include "../inc/sensors.h"

#define FREEMEM(d) if (d != NULL) { delete d; d = NULL; }
SensorController::~SensorController() {
    FREEMEM(i2c)
    FREEMEM(accelGyro)
    FREEMEM(magnetometer)
    FREEMEM(tempHumidity)
    FREEMEM(pressure)
    FREEMEM(irdaSensor)
}
#undef FREEMEM

void SensorController::initSensors() {
    LOG_VERBOSE("SensorController::initSensors");
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
float SensorController::readHumidity() {
    LOG_VERBOSE("SensorController::readHumidity");

    assert(tempHumidity != NULL);
    if (tempHumidity == NULL) {
        LOG_ERROR("Trying to do readHumidity while the sensor wasn't initialized.");
        return 0xFFFF;
    }

    float humidityValue;
    tempHumidity->reset();
    if (tempHumidity->getHumidity(&humidityValue) == 0)
        return humidityValue;
    else
        return 0xFFFF;
}

float SensorController::readTemperature() {
    LOG_VERBOSE("SensorController::readTemperature");

    assert(tempHumidity != NULL);
    if (tempHumidity == NULL) {
        LOG_ERROR("Trying to do readTemperature while the sensor wasn't initialized.");
        return 0xFFFF;
    }

    float tempValue;
    tempHumidity->reset();
    if (tempHumidity->getTemperature(&tempValue) == 0)
        return tempValue;
    else
        return 0xFFFF;
}

// LPS22HB
float SensorController::readPressure() {
    LOG_VERBOSE("SensorController::readPressure");

    assert(pressure != NULL);
    if (pressure == NULL) {
        LOG_ERROR("Trying to do readPressure while the sensor wasn't initialized.");
        return 0xFFFF;
    }

    float presureValue;
    if (pressure->getPressure(&presureValue) == 0)
        return presureValue;
    else
        return 0xFFFF;
}

// LIS2MDL
void SensorController::readMagnetometer(int *axes) {
    LOG_VERBOSE("SensorController::readMagnetometer");
    bool hasFailed = false;

    assert(magnetometer != NULL);
    if (magnetometer == NULL) {
        LOG_ERROR("Trying to do readMagnetometer while the sensor wasn't initialized.");
        hasFailed = true;
    }
    else if (magnetometer->getMAxes(axes) != 0) {
        hasFailed = true;
    }

    if (hasFailed) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

// LSM6DSL
void SensorController::readAccelerometer(int *axes) {
    LOG_VERBOSE("SensorController::readAccelerometer");
    bool hasFailed = false;

    assert(magnetometer != NULL);
    if (magnetometer == NULL) {
        LOG_ERROR("Trying to do readMagnetometer while the sensor wasn't initialized.");
        hasFailed = true;
    }

    if (accelGyro->getXAxes(axes) != 0) {
        hasFailed = true;
    }

    if (hasFailed) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

void SensorController::readGyroscope(int *axes) {
    LOG_VERBOSE("SensorController::readGyroscope");
    bool hasFailed = false;

    assert(accelGyro != NULL);
    if (accelGyro == NULL) {
        LOG_ERROR("Trying to do readGyroscope while the sensor wasn't initialized.");
        hasFailed = true;
    }

    if (accelGyro->getGAxes(axes) != 0) {
        hasFailed = true;
    }

    if (hasFailed) {
        axes[0] = 0xFFFF;
        axes[1] = 0xFFFF;
        axes[2] = 0xFFFF;
    }
}

bool SensorController::checkForShake() {
    int steps = 0;
    bool shake = false;

    assert(accelGyro != NULL);
    if (accelGyro == NULL) {
        LOG_ERROR("Trying to do checkForShake while the sensor wasn't initialized.");
        return false;
    }

    accelGyro->getStepCounter(&steps);
    if (steps > 1) {
        shake = true;
    }
    accelGyro->resetStepCounter();

    return shake;
}

// RGB LED
void SensorController::setLedColor(uint8_t red, uint8_t green, uint8_t blue) {
    rgbLed.setColor(red, green, blue);
}

void SensorController::turnLedOff() {
    LOG_VERBOSE("SensorController::turnLedOff");
    rgbLed.turnOff();
}

void SensorController::transmitIR() {
    LOG_VERBOSE("SensorController::transmitIR");

    assert(irdaSensor != NULL);
    if (irdaSensor == NULL) {
        LOG_ERROR("Trying to do transmitIR while the sensor wasn't initialized.");
        return;
    }

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