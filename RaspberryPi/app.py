# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

import iotc
from iotc import IOTConnectType, IOTLogLevel
from random import randint

deviceId = "DEVICE_ID"
scopeId = "SCOPE_ID"
deviceKey = "PRIMARY/SECONDARY device KEY"

iotc = iotc.Device(scopeId, deviceKey, deviceId, IOTConnectType.IOTC_CONNECT_SYMM_KEY)
iotc.setLogLevel(IOTLogLevel.IOTC_LOGGING_API_ONLY)

gCanSend = False
gCounter = 0

def onconnect(info):
  global gCanSend
  print("- [onconnect] => status:" + str(info.getStatusCode()))
  if info.getStatusCode() == 0:
     if iotc.isConnected():
       gCanSend = True
       iotc.sendProperty('{"dieNumber":' + str(randint(1, 6)) + '}')

def onmessagesent(info):
  print("\t- [onmessagesent] => " + str(info.getPayload()))

def oncommand(info):
  print("- [oncommand] => " + info.getTag() + " => " + str(info.getPayload()))

def onsettingsupdated(info):
  print("- [onsettingsupdated] => " + info.getTag() + " => " + info.getPayload())

iotc.on("ConnectionStatus", onconnect)
iotc.on("MessageSent", onmessagesent)
iotc.on("Command", oncommand)
iotc.on("SettingsUpdated", onsettingsupdated)

iotc.connect()

while iotc.isConnected():
  iotc.doNext() # do the async work needed to be done for MQTT
  if gCanSend == True:
    if gCounter % 20 == 0:
      gCounter = 0
      print("Sending telemetry..")
      iotc.sendTelemetry("{ \
\"temp\": " + str(randint(20, 45)) + ", \
\"pressure\": " + str(randint(900, 1100)) + ", \
\"humidity\": " + str(randint(50, 75)) + ", \
\"accelerometerX\": " + str(randint(2, 15)) + ", \
\"accelerometerY\": " + str(randint(3, 9)) + ", \
\"accelerometerZ\": " + str(randint(1, 4)) + ", \
\"magnetometerX\": " + str(randint(100, 120)) + ", \
\"magnetometerY\": " + str(randint(90, 110)) + ", \
\"magnetometerZ\": " + str(randint(200, 220)) + ", \
\"gyroscopeX\": " + str(randint(110, 130)) + ", \
\"gyroscopeY\": " + str(randint(50, 70)) + ", \
\"gyroscopeZ\": " + str(randint(190, 210)) + "}")

    gCounter += 1
