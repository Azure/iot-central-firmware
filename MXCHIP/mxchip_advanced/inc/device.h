// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef DEVICE_H
#define DEVICE_H

#include "globals.h"

// SINGLETON
class DeviceControl
{
    static DeviceState deviceState;
    DeviceControl() { }

public:
    static bool IsButtonClicked(unsigned char ulPin);
    static void incrementDeviceState();
    static DeviceState getDeviceState();
    static void showState();
    static void setState(DeviceState state);
};

#endif /* DEVICE_H */