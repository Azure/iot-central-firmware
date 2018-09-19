#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

import time
import json
from datetime import datetime
from threading import Thread
import random
import sys
import signal
import os
import configState
import deviceState
import utility
import logger
import registeredMethods
import globals
from provisioning_device_client import ProvisioningDeviceClient, ProvisioningTransportProvider, ProvisioningSecurityDeviceType, ProvisioningError, ProvisioningHttpProxyOptions
from provisioning_device_client_args import get_prov_client_opt, OptionError

from iotHub import IotHubClient
from counter import SenseHatCounter

GLOBAL_PROV_URI = "global.azure-devices-provisioning.net"
ID_SCOPE =        "<scope id>" # put the scope id here

SECURITY_DEVICE_TYPE = ProvisioningSecurityDeviceType.X509 # OR .SAS
PROTOCOL = ProvisioningTransportProvider.MQTT

iotHubClient = None
telemetryThread = None
stopProcess = False
kill_received = False
IOTHUB_URI =      None
IOTHUB_DID =      None

def initSensors():
    globals.sense.set_imu_config(True, True, True)

def readSensors():
    dataPayload = "{"
    if configState.config["sendData"]["temp"]:
        dataPayload += "\"temp\":" + str(globals.sense.get_temperature()) + ","

    if configState.config["sendData"]["humidity"]:
        dataPayload += "\"humidity\":" + str(globals.sense.get_humidity()) + ","

    if configState.config["sendData"]["pressure"]:
        dataPayload += "\"pressure\":" + str(globals.sense.get_pressure()) + ","

    if configState.config["sendData"]["gyro"]:
        gyro = globals.sense.get_gyroscope_raw()
        dataPayload += "\"gyroscopeX\":{x}, \"gyroscopeY\":{y}, \"gyroscopeZ\":{z},".format(**gyro)

    if configState.config["sendData"]["mag"]:
        mag = globals.sense.get_compass_raw()
        dataPayload += "\"magnetometerX\":{x}, \"magnetometerY\":{y}, \"magnetometerZ\":{z},".format(**mag)

    if configState.config["sendData"]["accel"]:
        accel = globals.sense.get_accelerometer_raw()
        dataPayload += "\"accelerometerX\":{x}, \"accelerometerY\":{y}, \"accelerometerZ\":{z},".format(**accel)

    dataPayload = dataPayload[:-1]
    dataPayload += "}"

    return dataPayload

def register_status_callback(reg_status, user_context):
    print ( "")
    print ( "Register status callback: ")
    print ( "reg_status = %s" % reg_status)
    print ( "user_context = %s" % user_context)
    print ( "")
    return


def register_device_callback(register_result, iothub_uri, device_id, user_context):
    global kill_received
    global iotHubClient
    global IOTHUB_URI
    global SECURITY_DEVICE_TYPE
    global PROTOCOL
    global IOTHUB_DID

    print ( "")
    print ( "Register device callback: " )
    print ( "   register_result = %s" % register_result)
    print ( "   iothub_uri = %s" % iothub_uri)
    print ( "   user_context = %s" % user_context)
    IOTHUB_URI = iothub_uri
    IOTHUB_DID = device_id

    if iothub_uri:
        print ( "")
        print ( "Device successfully registered!" )

        iotHubClient = IotHubClient(IOTHUB_URI, IOTHUB_DID, False if SECURITY_DEVICE_TYPE == ProvisioningSecurityDeviceType.X509 else True)

        # register a method for direct method execution
        # called with:
        #   iothub-explorer device-method <device-name> rainbow '{"timeInSec":10}' 3600
        iotHubClient.registerMethod("rainbow", registeredMethods.directMethod)

        # register a method for cloud to device (C2D) execution
        # called with:
        #   iothub-explorer send <device-name> '{"methodName":"message", "payload":{"text":"Hello World!!!", "color":[255,0,0]}}'
        iotHubClient.registerMethod("message", registeredMethods.cloudMessage)

        # register callbacks for desired properties expected
        iotHubClient.registerDesiredProperty("fanspeed", registeredMethods.fanSpeedDesiredChange)
        iotHubClient.registerDesiredProperty("setvoltage", registeredMethods.voltageDesiredChange)
        iotHubClient.registerDesiredProperty("setcurrent", registeredMethods.currentDesiredChange)
        iotHubClient.registerDesiredProperty("activateir", registeredMethods.irOnDesiredChange)

        while not kill_received:
            print "reading sensors\n"
            globals.display.increment(1)
            sensorData = readSensors()
            print "debug display\n"
            debugDisplay(sensorData)
            print "send data\n"
            sendDataToHub(sensorData)
            print "display show\n"
            globals.display.show()

            time.sleep(5)
    else:
        print ( "")
        print ( "Device registration failed!" )

    print ("done..")


def provision_device():
    global GLOBAL_PROV_URI
    global ID_SCOPE
    global SECURITY_DEVICE_TYPE
    global PROTOCOL

    try:
        provisioning_client = ProvisioningDeviceClient(GLOBAL_PROV_URI, ID_SCOPE, SECURITY_DEVICE_TYPE, PROTOCOL)

        version_str = provisioning_client.get_version_string()
        print ( "\nProvisioning API Version: %s\n" % version_str )

        provisioning_client.set_option("logtrace", True)

        provisioning_client.register_device(register_device_callback, None, register_status_callback, None)

        try:
            # Try Python 2.xx first
            raw_input("Press Enter to interrupt...\n")
        except:
            pass
            # Use Python 3.xx in the case of exception
            input("Press Enter to interrupt...\n")

    except ProvisioningError as provisioning_error:
        print ( "Unexpected error %s" % provisioning_error )
        return
    except KeyboardInterrupt:
        print ( "Provisioning Device Client sample stopped" )


def sendDataToHub(data):
    iotHubClient.send_message(data)


def debugDisplay(data):
    if configState.config["debug"]:
        logger.log("JSON payload to be sent:")
        jsonData = json.loads(data)
        logger.log(json.dumps(jsonData, indent=4, sort_keys=True))


def sendTelemetryStart():
    initSensors()
    provision_device()


def joystickStart():
    global kill_received

    while not kill_received:
        for event in globals.sense.stick.get_events():
            if event.action == "released":
                if event.direction == "up":
                    deviceState.setDeviceState("NORMAL")
                    color = [0,255,0]
                elif event.direction == "right":
                    deviceState.setDeviceState("CAUTION")
                    color = [255,165,0]
                elif event.direction == "down":
                    deviceState.setDeviceState("DANGER")
                    color = [255,0,0]
                elif event.direction == "left":
                    deviceState.setDeviceState("EXTREME_DANGER")
                    color = [0,0,255]
                globals.display.suspend(True)
                for i in range(0, 5):
                    globals.sense.clear(color)
                    time.sleep(0.1)
                    globals.sense.clear([0,0,0])
                    time.sleep(0.1)
                globals.display.suspend(False)
                globals.display.show()
                sendDataToHub("{{\"deviceState\":\"{0}\"}}".format(deviceState.getDeviceState()))

        time.sleep(1)


def accelerometerStart():
    global kill_received

    w = [255, 255, 255]
    b = [0, 0, 255]

    shake = []

    shake.append([
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w
    ])
    shake.append([
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,b,b,w,
        w,w,w,w,w,b,b,w,
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w,
        w,b,b,w,w,w,w,w,
        w,b,b,w,w,w,w,w,
        w,w,w,w,w,w,w,w
    ])
    shake.append([
        w,w,w,w,w,w,b,b,
        w,w,w,w,w,w,b,b,
        w,w,w,w,w,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,w,w,w,w,w,
        b,b,w,w,w,w,w,w,
        b,b,w,w,w,w,w,w
    ])
    shake.append([
        w,w,w,w,w,w,w,w,
        w,b,b,w,w,b,b,w,
        w,b,b,w,w,b,b,w,
        w,w,w,w,w,w,w,w,
        w,w,w,w,w,w,w,w,
        w,b,b,w,w,b,b,w,
        w,b,b,w,w,b,b,w,
        w,w,w,w,w,w,w,w
    ])
    shake.append([
        b,b,w,w,w,w,b,b,
        b,b,w,w,w,w,b,b,
        w,w,w,w,w,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,b,b,w,w,w,
        w,w,w,w,w,w,w,w,
        b,b,w,w,w,w,b,b,
        b,b,w,w,w,w,b,b
    ])
    shake.append([
        w,b,b,w,w,b,b,w,
        w,b,b,w,w,b,b,w,
        w,w,w,w,w,w,w,w,
        w,b,b,w,w,b,b,w,
        w,b,b,w,w,b,b,w,
        w,w,w,w,w,w,w,w,
        w,b,b,w,w,b,b,w,
        w,b,b,w,w,b,b,w
    ])

    while not kill_received:
        acceleration = globals.sense.get_accelerometer_raw()
        x = acceleration['x']
        y = acceleration['y']
        z = acceleration['z']

        x = abs(x)
        y = abs(y)
        z = abs(z)

        if x > 2 or y > 2 or z > 2:
            globals.display.suspend(True)
            globals.sense.clear()
            dieNumber = 0
            for i in range(0, 5):
                dieNumber = random.randint(0,5)
                globals.sense.set_pixels(shake[dieNumber])
                time.sleep(0.5)
            time.sleep(1)
            globals.display.suspend(False)
            globals.sense.clear()
            globals.display.show()

            #send the reported property for the rolled die number
            payload = "{{\"dieNumber\": {1}}}".format(datetime.now(), dieNumber + 1)
            iotHubClient.send_reported_property(payload)
        else:
            time.sleep(0.1)

def showIPOnDisplay(showIp):
    # show the IP on the display
    globals.sense.clear()

    if showIp:
        if deviceState.getIPaddress()[0] != "127.0.0.1" and deviceState.getIPaddress()[0] != "0.0.0.0": # check to see if loop back or not assigned
            globals.sense.show_message("Wifi IP: {0}".format(deviceState.getIPaddress()[0]), text_colour=[255,0,0])
        if deviceState.getIPaddress()[1] != "127.0.0.1" and deviceState.getIPaddress()[1] != "0.0.0.0": # check to see if loop back or not assigned
            globals.sense.show_message("Ethernet IP: {0}".format(deviceState.getIPaddress()[1]), text_colour=[0,255,0])

def main():
        #initialize logging
        logger.init()

        # pull in the saved config
        configState.init()

        # initialize globals
        globals.init()

        wlan0Ip = utility.getIpAddress('wlan0')
        eth0Ip = utility.getIpAddress('eth0')

        deviceState.init(globals.firmwareVersion)
        deviceState.setIPaddress(wlan0Ip, eth0Ip)
        showIPOnDisplay(configState.config["showIp"])

        threads = []

        # # start the telemetry thread
        telemetryThread = Thread(target=sendTelemetryStart, args=())
        telemetryThread.start()

        # start the joystick monitor thread
        joystickThread = Thread(target=joystickStart, args=())
        joystickThread.start()

        # # start the accelerometer monitor thread
        accelerometerThread = Thread(target=accelerometerStart, args=())
        accelerometerThread.start()

        global kill_received
        print("\nPress ctrl-c to stop the process")

        try:
            signal.pause()

        except KeyboardInterrupt:
            print("\nCtrl-c received! Sending kill to threads...")
            kill_received = True
            globals.sense.clear()


# kick off the process
main()
