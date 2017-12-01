# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. 

import os

logFile = None
logToConsole = False

def init():
    global logFile
    if logToConsole == False:
        logFile = open("../device.log", "a")


def log(message):
    global logFile

    # check if need to truncate
    if os.stat('../device.log').st_size >= 104857600:
        logFile.truncate(419430400)
    if logToConsole:
        print(message)
    else:
        logFile.write(message + "\n")
        logFile.flush()
