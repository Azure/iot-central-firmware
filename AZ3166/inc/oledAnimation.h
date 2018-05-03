// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef OLED_ANIMATION
#define OLED_ANIMATION_H

// SINGLETON
class AnimationController {
public:
    static void rollDieAnimation(int value);
    static void renderFrameToBuffer(unsigned char *screenBuffer, char *image);
    static void renderFrameToScreen(unsigned char *screenBuffer, int numFrames, bool center, unsigned frameDelay);
};

#endif /* OLED_ANIMATION_H */