// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/utility.h"
#include <AudioClass.h>


#include "../inc/sensors.h"
#include "../inc/stats.h"
#include "../inc/device.h"
#include "../inc/oledAnimation.h"

#include "../inc/fanSound.h"

// handler for the cloud to device (C2D) message
int cloudMessage(const char *payload, size_t size, char **response, size_t* resp_size) {
    Serial.println("Cloud to device (C2D) message recieved");

    // get parameters

    JSObject json(payload);
    const char * text = json.getStringByName("text");
    if (text == NULL) {
        LOG_ERROR("Object doesn't have a member 'text'");
        return 0;
    }

    // display the message on the screen
    Screen.clean();
    Screen.print(0, "New message:");
    Screen.print(1, text, true);
    delay(2000);

    return 200; /* status */
}

int directMethod(const char *payload, size_t size, char **response, size_t* resp_size) {
    // make the RGB LED color cycle
    unsigned int rgbColour[3];

    turnLedOff();
    delay(100);

    JSObject json(payload);
    double retval = json.getNumberByName("cycles");
    if (retval == INT_MAX || retval < 0) { // don't let overflow
        LOG_ERROR("'cycles' is not a number");
        return 0;
    }

    int cycles = (int) retval;
    for(int iter = 0; iter < cycles; iter++) {
        // Start off with red.
        rgbColour[0] = 255;
        rgbColour[1] = 0;
        rgbColour[2] = 0;

        // Choose the colours to increment and decrement.
        for (int decColour = 0; decColour < 3; decColour += 1) {
            int incColour = decColour == 2 ? 0 : decColour + 1;

            // cross-fade the two colours.
            for(int i = 0; i < 255; i += 1) {
                rgbColour[decColour] -= 1;
                rgbColour[incColour] += 1;

                setLedColor(rgbColour[0], rgbColour[1], rgbColour[2]);
                delay(5);
            }
        }
    }

    // return it to the status color
    delay(200);
    turnLedOff();
    delay(100);
    DeviceControl::showState();

    *response = (char*) GlobalConfig::completedString;
    return 200; /* status */
}

// this is the callback method for the fanSpeed desired property
int fanSpeedDesiredChange(const char *message, size_t size, char **response, size_t* resp_size) {
    char fan1[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', 'L', '.', '.', '.', '.', 'R', '.',
        '.', '.', 'L', '.', '.', 'R', '.', '.',
        '.', '.', '.', 'X', 'X', '.', '.', '.',
        '.', '.', '.', 'X', 'X', '.', '.', '.',
        '.', '.', 'R', '.', '.', 'L', '.', '.',
        '.', 'R', '.', '.', '.', '.', 'L', '.'};
    char fan2[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', 'V', 'v', '.', '.', '.',
        '.', '.', '.', 'V', 'v', '.', '.', '.',
        '.', 'H', 'H', 'X', 'X', 'H', 'H', '.',
        '.', 'h', 'h', 'X', 'X', 'h', 'h', '.',
        '.', '.', '.', 'V', 'v', '.', '.', '.',
        '.', '.', '.', 'V', 'v', '.', '.', '.'};

    char *fan[] = {fan1, fan2};

    animationInit(fan, 2, 64, 0, 0, true);

    Serial.println("fanSpeed desired property just got called");

    // turn on the fan - sound
    AudioClass& Audio = AudioClass::getInstance();
    Audio.startPlay(fanSoundData, FAN_SOUND_DATA_SIZE);

    // show the animation
    Screen.clean();
    for(int i = 0; i < 100; i++) {
        renderNextFrame();
    }

    incrementDesiredCount();

    *response = (char*) GlobalConfig::completedString;
    return 200; /* status */
}

int voltageDesiredChange(const char *message, size_t size, char **response, size_t* resp_size) {
    Serial.println("setVoltage desired property just got called");

    char voltage0[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.'};

    char voltage1[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G'};

    char voltage2[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G'};

    char voltage3[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G'};

    char voltage4[] = {
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G',
        'G', 'G', 'G', 'G', 'G', 'G', 'G', 'G'};

    char *voltage[] = {voltage0, voltage1, voltage2, voltage3, voltage4, voltage3, voltage2, voltage1, voltage0};

    animationInit(voltage, 9, 64, 0, 30, true);

    // show the animation
    Screen.clean();
    for(int i = 0; i < 54; i++) {
        renderNextFrame();
    }

    incrementDesiredCount();

    *response = (char*) GlobalConfig::completedString;
    return 200; /* status */
}

int currentDesiredChange(const char *message, size_t size, char **response, size_t* resp_size) {
    Serial.println("setCurrent desired property just got called");

    char current0[] = {
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.',
        '.', '.', '.', '.', '.', '.', '.', '.'};

    char current1[] = {
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.',
        'g', 'g', '.', '.', '.', '.', '.', '.'};

    char current2[] = {
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.',
        'g', 'g', 'g', 'g', '.', '.', '.', '.'};

    char current3[] = {
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.',
        'g', 'g', 'g', 'g', 'g', 'g', '.', '.'};

    char current4[] = {
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g',
        'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g'};

    char *current[] = {current0, current1, current2, current3, current4, current3, current2, current1, current0};

    animationInit(current, 9, 64, 0, 30, false);

    // show the animation
    Screen.clean();
    for(int i = 0; i < 54; i++) {
        renderNextFrame();
    }

    incrementDesiredCount();

    *response = (char*) GlobalConfig::completedString;
    return 200; /* status */
}

int irOnDesiredChange(const char *message, size_t size, char **response, size_t* resp_size) {
    Serial.println("activateIR desired property just got called");

    Screen.clean();
    Screen.print(0, "Firing IR beam");

    transmitIR();

    incrementDesiredCount();

    *response = (char*) GlobalConfig::completedString;
    return 200; /* status */
}