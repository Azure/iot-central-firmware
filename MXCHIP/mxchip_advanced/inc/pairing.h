// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef PAIRING_CONTROLLER
#define PAIRING_CONTROLLER


#include "globals.h"
#include "loop.h"

class WiFiUDP;
class PairingController : public LoopController
{
    bool setupCompleted;

    void listen();
    void pair();
    void cleanup();

    PairingController() : setupCompleted(false)
    {
        listen();
    }

private:
    bool startPairing();
    bool receiveData();
    bool storeConfig();
    bool broadcastSuccess();
    void errorAndReset();

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