// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include "globals.h"

class AutoString {
    static char buffer[STRING_BUFFER_512];
    static bool buffer_in_use; // no multithread support

    char * data;
    unsigned length;
    bool persistent;
    bool using_buffer;

public:
    AutoString(): data(NULL), length(0), persistent(false), using_buffer(false) { }

    AutoString(const char * str, unsigned lengthStr): persistent(false), using_buffer(false) {
        data = NULL;
        length = 0;

        initialize(str, lengthStr);
    }

    AutoString(unsigned lengthStr): persistent(false), using_buffer(false) {
        data = NULL;
        length = 0;

        alloc(lengthStr);
    }

    void initialize(const char * str, unsigned lengthStr) {
        if (str != NULL) {
            alloc(lengthStr + 1); // +1 for \0
            assert(data != NULL); // out of memory?

            memcpy(data, str, lengthStr);
            data[lengthStr] = 0;
            length = lengthStr;
        }
    }

    void alloc(unsigned lengthStr) {
        assert(lengthStr != 0 && data == NULL);

        if (buffer_in_use || lengthStr > STRING_BUFFER_512) {
            data = (char*) malloc(lengthStr /* * sizeof(char) */);
        } else {
            using_buffer = true;
            buffer_in_use = true;
            data = buffer;
        }
        memset(data, 0, lengthStr);

        length = lengthStr;
    }

    void set(unsigned index, char c) {
        assert(index < length && data != NULL);
        data[index] = c;
    }

    void clear() {
        if (data != NULL) {
            if (using_buffer) {
                using_buffer = false;
                buffer_in_use = false;
                memset(data, 0, STRING_BUFFER_512);
            } else {
                free (data);
            }
            data = NULL;
        }
    }

    ~AutoString() {
        if (!persistent) {
            this->clear();
        }
    }

    void makePersistent() {
        persistent = true;
        if (using_buffer) {
            AutoString tmp(data, length);
            tmp.makePersistent();
            clear();

            data = tmp.data;
            length = tmp.length;
        }
    }

    char* operator*() { return data; }

    unsigned getLength() { return length; }
    void setLength(unsigned l) { length = l; }
};

#include "../src/iotc/json.h"
using namespace AzureIOTC;

unsigned urldecode(const char * url, unsigned length, AutoString * outURL);
bool SyncTimeToNTP();

#endif /* INC_UTILITY_H */