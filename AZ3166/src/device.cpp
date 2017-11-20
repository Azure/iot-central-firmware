// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#include "Arduino.h"

#include "../inc/device.h"
#include "../inc/sensors.h"

static DeviceState deviceState = NORMAL;

bool IsButtonClicked(unsigned char ulPin)
{
    pinMode(ulPin, INPUT);
    int buttonState = digitalRead(ulPin);
    if(buttonState == LOW)
    {
        return true;
    }
    return false;
}

void showState() {
    switch(deviceState) {
        case NORMAL:
            setLedColor(0x00, 0xFF, 0x00);
            break;
        case CAUTION:
            setLedColor(0xFF, 0xC2, 0x00);
            break;
        case DANGER:
            setLedColor(0xFF, 0x00, 0x00);
            break;
        default:
            turnLedOff();
    }
}

DeviceState getDeviceState() {
    return deviceState;
}

void incrementDeviceState() {
    deviceState = (DeviceState)((deviceState + 1) % 3);
}