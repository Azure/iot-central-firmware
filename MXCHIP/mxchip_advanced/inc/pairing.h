// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef PAIRING_CONTROLLER
#define PAIRING_CONTROLLER

#include "AZ3166WiFi.h"
#include "AZ3166WiFiUdp.h"
#include "globals.h"
#include "loop.h"

class PairingController : public LoopController
{
    bool setupCompleted;
    char triggerMessage[PAIRING_TRIGGER_LENGTH];
    WiFiUDP udpClient;

    void listen();
    void pair();
    void cleanup();

    PairingController() : setupCompleted(false)
    {
        listen();
    }

public:
    static PairingController *New()
    {
        return new PairingController();
    }

    bool withTelemetry() override { return false; }

    void loop() override;

    ~PairingController()
    {
        cleanup();
    }
};

#endif // PAIRING_CONTROLLER