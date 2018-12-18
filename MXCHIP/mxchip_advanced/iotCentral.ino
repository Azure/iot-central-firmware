// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

/***

Full implementation of a device firmware for Azure IoT Central

Implements the following features:
•    Simple onboarding via a web UX
•    Simple device reset (press and hold the A & B buttons at the same time)
•    Display shows count of messages, errors, twin events, network information, and device name (cycle through screens with B button)
•    Telemetry sent for all onboard sensors (configurable)
•    State change telemetry sent when button A pressed and the device cycles through the three states (NORMAL, CAUTION, DANGER)
•    Reported twin property sent for double tap of device (uses accelerometer sensor data)
•    Desired twin property to simulate turning on a fan (fan sound plays from onboard headphone jack)
•    Cloud to device messages (supports sending a message to display on the screen)
•    Direct twin method calls (supports asking the device to play a rainbow sequence on the RGB LED)
•    LED status of network, Azure IoT send events, Azure IoT error events, and current device state (NORMAL=green, CAUTION=amber, DANGER=red)

Uses the following libraries:
•    Libraries installed by the MXChip IoT DevKit (https://microsoft.github.io/azure-iot-developer-kit/):
•    AureIoTHub - https://github.com/Azure/azure-iot-arduino
•    AzureIoTUtility - https://github.com/Azure/azure-iot-arduino-utility
•    AzureIoTProtocol_MQTT - https://github.com/Azure/azure-iot-arduino-protocol-mqtt

Third party libraries used:
•   Parson ( http://kgabis.github.com/parson/ ) Copyright (c) 2012 - 2017 Krzysztof Gabis

***/

#include "inc/application.h"


void setup()
{
    ApplicationController::initialize();
}

void loop()
{
    ApplicationController::loop();
}
