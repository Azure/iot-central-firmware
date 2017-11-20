// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef REGISTERED_METHOD_HANDLERS_H
#define REGISTERED_METHOD_HANDLERS_H

int cloudMessage(const char *payload, size_t size, char **response, size_t* resp_size); 
int directMethod(const char *payload, size_t size, char **response, size_t* resp_size);
int fanSpeedDesiredChange(const char *message, size_t size, char **response, size_t* resp_size);
int voltageDesiredChange(const char *message, size_t size, char **response, size_t* resp_size);
int currentDesiredChange(const char *message, size_t size, char **response, size_t* resp_size);
int irOnDesiredChange(const char *message, size_t size, char **response, size_t* resp_size);

    
#endif /* REGISTERED_METHOD_HANDLERS_H */