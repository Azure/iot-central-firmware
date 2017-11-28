import time
import globals
import pygame
import deviceState
import random

def voltageDesiredChange(payload):
    A = [[0, 0, 0] for x in range(8)]
    globals.display.suspend(True)
    globals.sense.clear()
    sleeptime = 0.3
    for i in range(0, 7):
        for x in range(0, 8):
            A[x] = [random.randint(10, 255), random.randint(10, 255), random.randint(10, 255)]
            bars = [
                A[7], A[7], A[7], A[7], A[7], A[7], A[7], A[7],
                A[6], A[6], A[6], A[6], A[6], A[6], A[6], A[6],
                A[5], A[5], A[5], A[5], A[5], A[5], A[5], A[5],
                A[4], A[4], A[4], A[4], A[4], A[4], A[4], A[4],
                A[3], A[3], A[3], A[3], A[3], A[3], A[3], A[3],
                A[2], A[2], A[2], A[2], A[2], A[2], A[2], A[2],
                A[1], A[1], A[1], A[1], A[1], A[1], A[1], A[1],
                A[0], A[0], A[0], A[0], A[0], A[0], A[0], A[0]
                ]
            globals.sense.set_pixels(bars)
            time.sleep(sleeptime) 
        A = [[0, 0, 0] for x in range(8)]

    globals.display.suspend(False)           
    globals.sense.clear()
    globals.display.show()
    return (200, "completed")

def currentDesiredChange(payload):
    A = [[0, 0, 0] for x in range(9)]
    globals.display.suspend(True)
    globals.sense.clear()
    sleeptime = 0.3
    for i in range(0, 7):
        for x in range(0, 8):
            A[x] = [random.randint(10, 255), random.randint(10, 255), random.randint(10, 255)]
            bars = [
                A[8], A[8], A[8], A[8], A[8], A[8], A[8], A[7],
                A[8], A[8], A[8], A[8], A[8], A[8], A[6], A[7],
                A[8], A[8], A[8], A[8], A[8], A[5], A[6], A[7],
                A[8], A[8], A[8], A[8], A[4], A[5], A[6], A[7],
                A[8], A[8], A[8], A[3], A[4], A[5], A[6], A[7],
                A[8], A[8], A[2], A[3], A[4], A[5], A[6], A[7],
                A[8], A[1], A[2], A[3], A[4], A[5], A[6], A[7],
                A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7]
                ]
            globals.sense.set_pixels(bars)
            time.sleep(sleeptime) 
        A = [[0, 0, 0] for x in range(9)]
        
    globals.display.suspend(False)           
    globals.sense.clear()
    globals.display.show()
    return (200, "completed")

def irOnDesiredChange(payload):
    r = random.randint(10, 255)
    g = random.randint(10, 255)
    b = random.randint(10, 255)
    ir1 = [
        [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b],
        [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b]
    ]

    r = random.randint(10, 255)
    g = random.randint(10, 255)
    b = random.randint(10, 255)
    ir2 = [
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [0, 0, 0],
        [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0],
        [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0],
        [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0],
        [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0],
        [0, 0, 0], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]
    ]

    r = random.randint(10, 255)
    g = random.randint(10, 255)
    b = random.randint(10, 255)
    ir3 = [
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0], [r, g, b], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [r, g, b], [r, g, b], [r, g, b], [r, g, b], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]
    ]

    r = random.randint(10, 255)
    g = random.randint(10, 255)
    b = random.randint(10, 255)
    ir4 = [
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [r, g, b], [r, g, b], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]
    ]

    globals.display.suspend(True)
    globals.sense.clear()
    sleeptime = 0.3
    for i in range(0, 7):
        globals.sense.set_pixels(ir1)
        time.sleep(sleeptime)
        globals.sense.set_pixels(ir2)
        time.sleep(sleeptime)
        globals.sense.set_pixels(ir3)
        time.sleep(sleeptime)    
        globals.sense.set_pixels(ir4)
        time.sleep(sleeptime) 

    globals.display.suspend(False)           
    globals.sense.clear()
    globals.display.show()

    return (200, "completed")

def fanSpeedDesiredChange(payload):
    twin = deviceState.getLastTwinAsObject()
    currentValue = 0
    if "desired" in twin and "fanSpeed" in twin["desired"]:
        currentValue = twin["desired"]["fanSpeed"]["value"]

    faster = True
    if (currentValue > payload["value"]):
        faster = False

    pygame.init()
    pygame.mixer.init()
    fanSound=pygame.mixer.Sound("../content/fan.wav")
    fanSound.set_volume(10.0)
    fanSound.play(5)

    fan1 = [
        [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255],
        [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 255], [0, 0, 255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 255], [0, 0, 255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0],
        [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255]
    ]

    fan2 = [
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [255,255,255],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 255], [0, 0, 255], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 255], [0, 0, 255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [255,255,255], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0]
    ]

    fan3 = [
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0],
        [255,255,255], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 255], [0, 0, 255], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 255], [0, 0, 255], [255,255,255], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [255,255,255], [255,255,255],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0],
        [0, 0, 0], [0, 0, 0], [255,255,255], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]
    ]

    globals.display.suspend(True)
    globals.sense.clear()
    topRange = 0
    if faster:
        sleeptime = 0.3
        topRange = 25
    else:
        sleeptime = 0.075
        topRange = 10

    for i in range(0, topRange):
        globals.sense.set_pixels(fan1)
        time.sleep(sleeptime)
        globals.sense.set_pixels(fan2)
        time.sleep(sleeptime)
        globals.sense.set_pixels(fan3)
        time.sleep(sleeptime)  
        if faster: 
            if sleeptime >= 0.075:
                sleeptime -= 0.025   
        else:
            if sleeptime <= 0.3:
                sleeptime += 0.025  
    fanSound.stop()
    globals.display.suspend(False)           
    globals.sense.clear()
    globals.display.show()
    
    return (200, "completed")


def cloudMessage(payload):
    globals.display.suspend(True)
    globals.sense.clear()
    color = [255,255,255]
    if "color" in payload:
        color = payload["color"]
    globals.sense.show_message(payload["text"], text_colour=color)
    globals.display.suspend(False)
    globals.sense.clear()
    globals.display.show()


def directMethod(payload):
    timeInSec = payload["timeInSec"]

    pixels = [
        [255, 0, 0], [255, 0, 0], [255, 87, 0], [255, 196, 0], [205, 255, 0], [95, 255, 0], [0, 255, 13], [0, 255, 122],
        [255, 0, 0], [255, 96, 0], [255, 205, 0], [196, 255, 0], [87, 255, 0], [0, 255, 22], [0, 255, 131], [0, 255, 240],
        [255, 105, 0], [255, 214, 0], [187, 255, 0], [78, 255, 0], [0, 255, 30], [0, 255, 140], [0, 255, 248], [0, 152, 255],
        [255, 223, 0], [178, 255, 0], [70, 255, 0], [0, 255, 40], [0, 255, 148], [0, 253, 255], [0, 144, 255], [0, 34, 255],
        [170, 255, 0], [61, 255, 0], [0, 255, 48], [0, 255, 157], [0, 243, 255], [0, 134, 255], [0, 26, 255], [83, 0, 255],
        [52, 255, 0], [0, 255, 57], [0, 255, 166], [0, 235, 255], [0, 126, 255], [0, 17, 255], [92, 0, 255], [201, 0, 255],
        [0, 255, 66], [0, 255, 174], [0, 226, 255], [0, 117, 255], [0, 8, 255], [100, 0, 255], [210, 0, 255], [255, 0, 192],
        [0, 255, 183], [0, 217, 255], [0, 109, 255], [0, 0, 255], [110, 0, 255], [218, 0, 255], [255, 0, 183], [255, 0, 74]
    ]

    def next_colour(pix):
        r = pix[0]
        g = pix[1]
        b = pix[2]

        if (r == 255 and g < 255 and b == 0):
            g += 1

        if (g == 255 and r > 0 and b == 0):
            r -= 1

        if (g == 255 and b < 255 and r == 0):
            b += 1

        if (b == 255 and g > 0 and r == 0):
            g -= 1

        if (b == 255 and r < 255 and g == 0):
            r += 1

        if (r == 255 and b > 0 and g == 0):
            b -= 1

        pix[0] = r
        pix[1] = g
        pix[2] = b

    globals.sense.clear()
    globals.display.suspend(True)

    timeout_start = time.time()
    while time.time() < timeout_start + timeInSec:
        for pix in pixels:
            next_colour(pix)

        globals.sense.set_pixels(pixels)

    globals.sense.clear()
    globals.display.suspend(False)
    globals.display.show()

    return (200, "completed")


