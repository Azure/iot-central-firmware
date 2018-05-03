// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef ONBOARDING_CONTROLLER
#define ONBOARDING_CONTROLLER

#include "loop.h"

class WiFiClient;
class AzWebServer;

class OnboardingController: public LoopController {
    bool setupCompleted;
    bool pincodePasses;
    AzWebServer * webServer;

    void initializeConfigurationSetup();
    void processStartRequest(WiFiClient &client);
    void processResultRequest(WiFiClient &client, String &request);

    OnboardingController(): setupCompleted(false), pincodePasses(false),
                            webServer(NULL)
    {
        initializeConfigurationSetup();
    }

    void cleanup();

public:

    static OnboardingController * New() {
        return new OnboardingController();
    }

    bool withTelemetry() override { return false; }

    void loop() override;

    ~OnboardingController() {
        cleanup();
    }
};

#endif // ONBOARDING_CONTROLLER