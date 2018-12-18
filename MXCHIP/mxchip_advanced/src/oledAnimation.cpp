// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"

#include "OledDisplay.h"

#include "../inc/oledAnimation.h"

unsigned char block[]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char blockGap[]  = {0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E};
unsigned char blockVGap[]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
unsigned char cross[]  = {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};
unsigned char diagRL[] = {0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03};
unsigned char diagLR[] = {0x03, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC0};
unsigned char horzB[]  = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
unsigned char horzT[]  = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
unsigned char vertL[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
unsigned char vertR[]  = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char circle[] = {0x18, 0x7E, 0x7E, 0xFF, 0xFF, 0x7E, 0x7E, 0x18};
unsigned char clear[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char borderBottom[]  = {0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0};
unsigned char borderTop[]  = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
unsigned char borderLeft[]  = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char borderRight[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};
unsigned char cornerLB[]  = {0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0};
unsigned char cornerRB[]  = {0xC0, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF};
unsigned char cornerLT[]  = {0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
unsigned char cornerRT[]  = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF};
unsigned char circleLT[]  = {0x00, 0xF0, 0xF8, 0xFC, 0xFC, 0xFE, 0xFE, 0xFE};
unsigned char circleRT[]  = {0xFE, 0xFE, 0xFE, 0xFC, 0xFC, 0xF8, 0xF0, 0x00};
unsigned char circleLB[]  = {0x00, 0x0F, 0x1F, 0x3F, 0x3F, 0x7F, 0x7F, 0x7F};
unsigned char circleRB[]  = {0x7F, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x0F, 0x00};

void AnimationController::renderFrameToBuffer(unsigned char *screenBuffer, char *image) {
    int columnPad = 3;
    int colLimit = 8;

    for(int y = 0; y < 8; y++) {
        for(int x = 0; x < colLimit; x++) {
            if (image[(y * colLimit) + x] == 'B')
                memcpy(screenBuffer + columnPad, block, 8);
            else if (image[(y * colLimit) + x] == '.')
                memcpy(screenBuffer + columnPad, clear, 8);
            else if (image[(y * colLimit) + x] == 'G')
                memcpy(screenBuffer + columnPad, blockGap, 8);
            else if (image[(y * colLimit) + x] == 'g')
                memcpy(screenBuffer + columnPad, blockVGap, 8);
            else if (image[(y * colLimit) + x] == 'X')
                memcpy(screenBuffer + columnPad, cross, 8);
            else if (image[(y * colLimit) + x] == 'L')
                memcpy(screenBuffer + columnPad, diagLR, 8);
            else if (image[(y * colLimit) + x] == 'R')
                memcpy(screenBuffer + columnPad, diagRL, 8);
            else if (image[(y * colLimit) + x] == 'H')
                memcpy(screenBuffer + columnPad, horzT, 8);
            else if (image[(y * colLimit) + x] == 'h')
                memcpy(screenBuffer + columnPad, horzB, 8);
            else if (image[(y * colLimit) + x] == 'V')
                memcpy(screenBuffer + columnPad, vertL, 8);
            else if (image[(y * colLimit) + x] == 'v')
                memcpy(screenBuffer + columnPad, vertR, 8);
            else if (image[(y * colLimit) + x] == 'O')
                memcpy(screenBuffer + columnPad, circle, 8);
            else if (image[(y * colLimit) + x] == 'T')
                memcpy(screenBuffer + columnPad, borderTop, 8);
            else if (image[(y * colLimit) + x] == 'b')
                memcpy(screenBuffer + columnPad, borderBottom, 8);
            else if (image[(y * colLimit) + x] == '<')
                memcpy(screenBuffer + columnPad, borderLeft, 8);
            else if (image[(y * colLimit) + x] == '>')
                memcpy(screenBuffer + columnPad, borderRight, 8);
            else if (image[(y * colLimit) + x] == '1')
                memcpy(screenBuffer + columnPad, cornerLT, 8);
            else if (image[(y * colLimit) + x] == '2')
                memcpy(screenBuffer + columnPad, cornerRT, 8);
            else if (image[(y * colLimit) + x] == '3')
                memcpy(screenBuffer + columnPad, cornerLB, 8);
            else if (image[(y * colLimit) + x] == '4')
                memcpy(screenBuffer + columnPad, cornerRB, 8);
            else if (image[(y * colLimit) + x] == '!')
                memcpy(screenBuffer + columnPad, circleLT, 8);
            else if (image[(y * colLimit) + x] == '@')
                memcpy(screenBuffer + columnPad, circleRT, 8);
            else if (image[(y * colLimit) + x] == '#')
                memcpy(screenBuffer + columnPad, circleLB, 8);
            else if (image[(y * colLimit) + x] == '$')
                memcpy(screenBuffer + columnPad, circleRB, 8);

            columnPad = columnPad + 8;
        }
        columnPad = columnPad + 8;
    }
}

void AnimationController::renderFrameToScreen(unsigned char *screenBuffer,
    int numFrames, bool center, unsigned frameDelay) {

    const int width = 64;
    unsigned char xs = 0;
    unsigned char ys = 0;
    unsigned char xe = 0;
    unsigned char ye = 8;

    int centerPad = 0;
    if (center)
        centerPad = 32;

    for (int i = 0; i < numFrames; i++) {
        unsigned char * buffer = screenBuffer + (i * OLED_SINGLE_FRAME_BUFFER);

        xe = width + 8 + centerPad;
        Screen.draw(xs + centerPad, ys, xe, ye, buffer);

        if (frameDelay > 0) {
            delay(frameDelay);
        }
    }
}

void AnimationController::rollDieAnimation(int value) {
    char die1[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die2[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '.', '.', '>',
        '<', '#', '$', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '!', '@', '>',
        '<', '.', '.', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die3[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '.', '.', '>',
        '<', '#', '$', '.', '.', '.', '.', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '.', '.', '.', '.', '!', '@', '>',
        '<', '.', '.', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die4[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '.', '.', '.', '.', '.', '.', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die5[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '.', '.', '!', '@', '.', '.', '>',
        '<', '.', '.', '#', '$', '.', '.', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char die6[] = {
        '1', 'T', 'T', 'T', 'T', 'T', 'T', '2',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '<', '!', '@', '.', '.', '!', '@', '>',
        '<', '#', '$', '.', '.', '#', '$', '>',
        '3', 'b', 'b', 'b', 'b', 'b', 'b', '4'};

    char *die[] = { die1, die2, die3, die4, die5, die6 };
    const int numberOfRolls = 3;
    char *roll[numberOfRolls + 1];
    char uniqueRolls[6] = {0};
    uniqueRolls[value - 1] = 1;

    for (int i = 0; i < numberOfRolls; i++) {
re_roll:
        int dieRoll = random(0, 6);
        if (uniqueRolls[dieRoll] != 0) goto re_roll;
        uniqueRolls[dieRoll] = 1;

        roll[i] = die[dieRoll];
    }
    roll[numberOfRolls] = die[value - 1];

    // detach stack alloc
    {
        unsigned char buffer[(numberOfRolls + 1) * OLED_SINGLE_FRAME_BUFFER] = {0};
        Screen.clean();
        for(int i = 0; i < numberOfRolls + 1; i++) {
            renderFrameToBuffer(buffer + (i * OLED_SINGLE_FRAME_BUFFER), roll[i]);
        }

        // show the animation
        renderFrameToScreen(buffer, numberOfRolls + 1, true, 100);
    }

    delay(1200); // to keep last number on the screen longer
}