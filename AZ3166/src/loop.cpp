// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/loop.h"

LoopController::LoopController() {
    initializeCompleted = false;
}

bool LoopController::wasInitializeCompleted() {
    return initializeCompleted;
}

LoopController::~LoopController() {
    initializeCompleted = false;
}