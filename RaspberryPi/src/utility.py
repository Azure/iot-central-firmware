# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. 

import socket
import fcntl
import struct

def getIpAddress(ifname):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', bytes(ifname[:15], "utf-8"))
        )[20:24])
    except:
        return "0.0.0.0"

def getDeviceName(connectionString):
    idx = connectionString.index(";DeviceId=") + 10
    name = connectionString[idx: connectionString.index(";", idx)]
    return name
