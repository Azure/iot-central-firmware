// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef LOOP_CONTROLLER
#define LOOP_CONTROLLER

class LoopController
{
protected:
    bool initializeCompleted;

public:
    LoopController();

    bool wasInitializeCompleted();

    virtual void loop() = 0;
    virtual bool withTelemetry() = 0;
    virtual ~LoopController();
};

#endif // LOOP_CONTROLLER