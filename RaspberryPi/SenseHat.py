# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

import iotc
from iotc import IOTConnectType, IOTLogLevel
from random import randint
from sense_hat import SenseHat


deviceId = "DEVICE_ID"
scopeId = "SCOPE_ID"
deviceKey = "PRIMARY/SECONDARY device KEY"

sense = SenseHat()
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
    if gCounter % 5 == 0:
      gCounter = 0
      #get weather data from sense hat
      temp = sense.get_temperature()
      press = sense.get_pressure()
      hum = sense.get_humidity()

      print("Sending telemetry..")
      iotc.sendTelemetry("{ \
      \"temp\": " + str(temp) + ", \
      \"pressure\": " + str(press) + ", \
      \"humidity\": " + str(hum) +  " }")

      temp = round(temp, 2)
      press = round(press, 2)
      hum = round(hum, 2)

      #show weather sensor data on sense hat led matrix
      sense.show_message("Temp: " + str(temp) + " Pressure: " +  str(press) + " Humidity: " +  str(hum))
     
    gCounter += 1
