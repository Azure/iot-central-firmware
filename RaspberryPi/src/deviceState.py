# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

import datetime
import json
from collections import OrderedDict

def init(firmwareVersion):
    global device
    device = {}
    device["name"] = "AzureIoTCentralSample"
    device["firmwareVersion"] = firmwareVersion
    device["deviceState"] = "NORMAL"
    device["sentCount"] = 0
    device["errorCount"] = 0
    device["desiredCount"] = 0
    device["reportedCount"] = 0
    device["directCount"] = 0
    device["c2dCount"] = 0
    device["lastSend"] = datetime.datetime.fromordinal(1)
    device["lastPayload"] = "{}"
    device["lastTwin"] = "{}"
    device["wifiIP"] = ""
    device["etherIP"] = ""

def getName():
    global device
    return device["name"]

def setName(value):
    global device
    device["name"] = value

def getFirmwareVersion():
    global device
    return device["firmwareVersion"]

def setFirmwareVersion(value):
    global device
    device["firmwareVersion"] = value

def getDeviceState():
    global device
    return device["deviceState"]

def setDeviceState(value):
    global device
    device["deviceState"] = value

def getSentCount():
    global device
    return device["sentCount"]

def incSentCount():
    global device
    device["sentCount"] += 1

def setSentCount(value):
    global device
    device["sentCount"] = value

def getErrorCount():
    global device
    return device["errorCount"]

def incErrorCount():
    global device
    device["errorCount"] += 1

def setErrorCount(value):
    global device
    device["errorCount"] = value

def getDesiredCount():
    global device
    return device["desiredCount"]

def incDesiredCount():
    global device
    device["desiredCount"] += 1

def setDesiredCount(value):
    global device
    device["desiredCount"] = value

def getReportedCount():
    global device
    return device["reportedCount"]

def incReportedCount():
    global device
    device["reportedCount"] += 1

def setReportedCount(value):
    global device
    device["reportedCount"] = value

def getDirectCount():
    global device
    return device["directCount"]

def incDirectCount():
    global device
    device["directCount"] += 1

def setDirectCount(value):
    global device
    device["directCount"] = value

def getC2dCount():
    global device
    return device["c2dCount"]

def incC2dCount():
    global device
    device["c2dCount"] += 1

def setC2dCount(value):
    global device
    device["c2dCount"] = value

def getLastSend():
    global device
    return device["lastSend"]

def setLastSend(value):
    global device
    device["lastSend"] = value

def getLastPayload():
    global device
    parsed = json.loads(device["lastPayload"])
    return json.dumps(parsed, indent=4, sort_keys=True).replace("\n", "<br>").replace(" ", "&nbsp;")

def setLastPayload(value):
    global device
    device["lastPayload"] = value

def getLastTwin():
    global device
    parsed = json.loads(device["lastTwin"])
    return json.dumps(parsed, indent=4, sort_keys=True).replace("\n", "<br>").replace(" ", "&nbsp;")

def getLastTwinAsObject():
    global device
    return json.loads(device["lastTwin"])

def setLastTwin(value):
    global device
    device["lastTwin"] = value

def patchLastTwin(payload, desired):
    global device
    if device["lastTwin"] == "{}":
        return    # cannot patch the twin as it has not been received yet

    current = json.loads(device["lastTwin"], object_pairs_hook=OrderedDict)
    patch = json.loads(payload, object_pairs_hook=OrderedDict)
    part = "reported"
    if desired:
        part = "desired"
    propertyName = list(patch)[0]
    current[part][propertyName] = patch[propertyName]
    if desired:
        current[part]["$version"] = patch["$version"]
    else:
        current[part]["$version"] += 1
    device["lastTwin"] = json.dumps(current)

def getIPaddress():
    global device
    return (device["wifiIP"], device["etherIP"])

def setIPaddress(wifiIp, etherIp):
    global device
    device["wifiIP"] = wifiIp
    device["etherIP"] = etherIp