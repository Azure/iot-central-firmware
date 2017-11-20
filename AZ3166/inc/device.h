// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef DEVICE_H
#define DEVICE_H

typedef enum {NORMAL, CAUTION, DANGER} DeviceState;

bool IsButtonClicked(unsigned char ulPin);

void incrementDeviceState();
DeviceState getDeviceState();
void showState();

#endif /* DEVICE_H */