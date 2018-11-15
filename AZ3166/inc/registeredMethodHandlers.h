// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef REGISTERED_METHOD_HANDLERS_H
#define REGISTERED_METHOD_HANDLERS_H

int dmEcho(const char *payload, size_t size);
int dmCountdown(const char *payload, size_t size);
int fanSpeedDesiredChange(const char *message, size_t size);
int voltageDesiredChange(const char *message, size_t size);
int currentDesiredChange(const char *message, size_t size);
int irOnDesiredChange(const char *message, size_t size);


#endif /* REGISTERED_METHOD_HANDLERS_H */