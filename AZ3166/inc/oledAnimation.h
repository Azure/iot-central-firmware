// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef OLED_ANIMATION
#define OLED_ANIMATION_H

void animationInit(char **frames, int maxFrames, int width, int moveLimit, int frameDelay, bool center);
void clearScreen();
void renderNextFrame();
void animationEnd();

#endif /* OLED_ANIMATION_H */