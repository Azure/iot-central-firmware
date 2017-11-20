import configState
from counter import SenseHatCounter

sense = None
display = None
firmwareVersion = "0.9-AIoTC"

def init():
    if configState.config["simulateSenseHat"]:
        from sense_emu import SenseHat
    else:
        from sense_hat import SenseHat

    global sense
    sense = SenseHat()

    global display
    display = SenseHatCounter()
