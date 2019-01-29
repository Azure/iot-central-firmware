// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include "globals.h"

#include "../src/iotc/common/string_buffer.h"
#include "../src/iotc/common/json.h"
using namespace AzureIOT;

unsigned urldecode(const char * url, unsigned length, StringBuffer * outURL);
bool SyncTimeToNTP();

#endif /* INC_UTILITY_H */