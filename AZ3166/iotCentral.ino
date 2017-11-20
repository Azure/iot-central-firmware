// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

/***
 
Full implementation of a device firmware for Azure IoT Central

Implements the following features:
 •  Simple onboarding via a web UX
 •	Simple device reset (press and hold the A & B buttons at the same time)
 •	Display shows count of messages, errors, twin events, network information, and device name (cycle through screens with B button)
 •	Telemetry sent for all onboard sensors (configurable)
 •	State change telemetry sent when button A pressed and the device cycles through the three states (NORMAL, CAUTION, DANGER)
 •	Reported twin property sent for double tap of device (uses accelerometer sensor data)
 •	Desired twin property to simulate turning on a fan (fan sound plays from onboard headphone jack)
 •	Cloud to device messages (supports sending a message to display on the screen)
 •	Direct twin method calls (supports asking the device to play a rainbow sequence on the RGB LED)
 •	LED status of network, Azure IoT send events, Azure IoT error events, and current device state (NORMAL=green, CAUTION=amber, DANGER=red)

Uses the following libraries:
 •	Libraries installed by the MXChip IoT DevKit (https://microsoft.github.io/azure-iot-developer-kit/):
     •	zureIoTHub - https://github.com/Azure/azure-iot-arduino
     •	AzureIoTUtility - https://github.com/Azure/azure-iot-arduino-utility
     •	AzureIoTProtocol_MQTT - https://github.com/Azure/azure-iot-arduino-protocol-mqtt

•   Third party libraries used:
     •	ArduinoJson - https://bblanchon.github.io/ArduinoJson/

***/

#include "Arduino.h"
#include "EEPROMInterface.h"

#include "inc/iotCentral.h"
#include "inc/main_initialize.h"
#include "inc/main_telemetry.h"
#include "inc/config.h"
#include "inc/device.h"

bool configured = false;
String iotCentralConfig;

void setup()
{
    Serial.begin(250000);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_AZURE, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    iotCentralConfig = readIotCentralConfig();

    if (iotCentralConfig[0] == 0x00) { 
        (void)Serial.printf("No configuration found entering config mode.\r\n");
        initializeSetup();
    } else {
        (void)Serial.printf("Configuration found entering telemetry mode.\r\n");
        configured = true;
        telemetrySetup(iotCentralConfig);
    }
}

void loop()
{
    // reset the device if the A and B buttons are both pressed and held
    if (IsButtonClicked(USER_BUTTON_A) && IsButtonClicked(USER_BUTTON_B)) {
        Screen.clean();
        Screen.print(0, "Device resetting");
        clearAllConfig();
        
        if (configured) {
            telemetryCleanup();
        } else {
            initializeCleanup();
        }

        configured = false;
        delay(1000);  //artificial pause
        Screen.clean();
        Screen.print(0, "Device is reset");
        Screen.print(1, "Press reset");
        Screen.print(2, "to configure");

        //initializeSetup();
    }

	if (configured) {
        telemetryLoop();
    } else {
        initializeLoop();
    }
}
