// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef APPLICATION_CONTROLLER
#define APPLICATION_CONTROLLER

// SINGLETON
class ApplicationController {
    ApplicationController() { }
public:
    static bool initialize();
    static bool loop();
};

#endif // APPLICATION_CONTROLLER