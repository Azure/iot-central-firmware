# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. 


import globals
import sys

class SenseHatCounter:
    colors = [(255,0,0), (0,255,0), (0,0,255), (255,0,255), (0,255,255), (255,255,0)]
    maxCount = 999999

    def __init__(self):
        self.counter = 0
        globals.sense.clear(0, 0, 0)
        self.shouldShow = True

    def reset(self):
        self.counter = 0

    @property
    def count(self):
        return self.counter

    @count.setter
    def count(self, value):
        if value > self.maxCount:
            self.counter = 0
        else:
            self.counter = value

    def increment(self, increment = 1):
        if self.counter + increment > self.maxCount:
            self.counter = 0
            globals.sense.clear((0,0,0))
        else:
            self.counter += increment

    def decrement(self):
        self.counter -= 1

    def suspend(self, show):
        self.shouldShow = not show

    def show(self):
        if self.shouldShow:
            currentCount = self.counter
            row = 0
            col = 0
            colorIdx = 0
            while currentCount > 0:
                divider = currentCount % 10
                if colorIdx < 4:
                    if divider == 0:
                        # clear the row of pixels
                        for x in range(3):
                            globals.sense.set_pixel(x + col, row, 0, 0, 0)
                        for x in range(3):
                            globals.sense.set_pixel(x + col, row+1, 0, 0, 0) 
                        for x in range(3):
                            globals.sense.set_pixel(x + col, row+2, 0, 0, 0) 
                    else:
                        # display the count as pixels
                        if divider > 0:
                            for x in range(min(divider, 3)):
                                globals.sense.set_pixel(x + col, row, self.colors[colorIdx])
                        if divider > 3:
                            for x in range(min(divider - 3, 3)):
                                globals.sense.set_pixel(x + col, row+1, self.colors[colorIdx]) 
                        if divider > 6:
                            for x in range(min(divider - 6, 3)):
                                globals.sense.set_pixel(x + col, row+2, self.colors[colorIdx]) 
                elif colorIdx == 4:
                    if divider == 0:
                        color = (0,0,0)
                        for x in range(5):
                            globals.sense.set_pixel(x + 3, 6, (0,0,0))
                        for x in range(4):
                            globals.sense.set_pixel(6, 5 - x, (0,0,0))
                    else:
                        for x in range(divider):
                            if x < 5:
                                globals.sense.set_pixel(x + 2, 6, self.colors[colorIdx])
                            else:
                                globals.sense.set_pixel(6, 5 - (x - 5), self.colors[colorIdx])
                elif colorIdx == 5:
                    if divider == 0:
                        color = (0,0,0)
                        for x in range(5):
                            globals.sense.set_pixel(x + 3, 7, (0,0,0))
                        for x in range(4):
                            globals.sense.set_pixel(7, 6 - x, (0,0,0))
                    else:
                        for x in range(divider):
                            if x < 5:
                                globals.sense.set_pixel(x + 3, 7, self.colors[colorIdx])
                            else:
                                globals.sense.set_pixel(7, 6 - (x - 5), self.colors[colorIdx])                
                colorIdx += 1
                currentCount //= 10
                if colorIdx % 2 == 0:
                    row += 3
                    col = 0
                else:
                    col += 3
