// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef WATCHDOG_CONTROLLER
#define WATCHDOG_CONTROLLER

class Watchdog;

// SINGLETON
class WatchdogController
{
    static Watchdog watchdog;

public:
    static void initialize(int timer = 32767);
    static void reset(bool setState = false);
};

#endif // WATCHDOG_CONTROLLER
