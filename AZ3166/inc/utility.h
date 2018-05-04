// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include "globals.h"
#include <parson/parson.h>

class AutoString {
    char * data;
    unsigned length;
    bool persistent;

public:
    AutoString(): data(NULL), length(0), persistent(false) { }

    AutoString(const char * str, unsigned lengthStr): persistent(false) {
        data = NULL;
        length = 0;

        initialize(str, lengthStr);
    }

    AutoString(unsigned lengthStr): persistent(false) {
        data = NULL;
        length = 0;

        alloc(lengthStr);
    }

    void initialize(const char * str, unsigned lengthStr) {
        if (str != NULL) {
            data = strdup(str);
            length = lengthStr;
            assert(data != NULL);
        }
    }

    void alloc(unsigned lengthStr) {
        assert(lengthStr != 0 && data == NULL);
        data = (char*) malloc(lengthStr + 1 /* * sizeof(char) */);
        assert(data != NULL);
        memset(data, 0, lengthStr);
        length = lengthStr;
    }

    void set(unsigned index, char c) {
        assert(index < length && data != NULL);
        data[index] = c;
    }

    void clear() {
        if (data != NULL) {
            free (data);
        }
    }

    ~AutoString() {
        if (!persistent) {
            this->clear();
        }
    }

    void makePersistent() { persistent = true; }

    char* operator*() { return data; }

    unsigned getLength() { return length; }
};

class JSObject
{
private:
    JSON_Value* value;
    JSON_Object* object;
    bool isSubObject;

    JSON_Object* toObject() {
        object = json_value_get_object(value);
        if (object == NULL) {
            LOG_ERROR("JSON value is not an object");
            return NULL;
        }
        return object;
    }
public:
    JSObject(): value(NULL), object(NULL), isSubObject(false) { }

    JSObject(const char * json_string) : isSubObject(false) {
        value = json_parse_string(json_string);
        if (value == NULL) {
            LOG_ERROR("parsing JSON failed");
        }

        object = toObject();
        if (object == NULL) {
            LOG_ERROR("json data: %s", json_string);
        }
    }

    ~JSObject();

    const char * toString();

    bool getObjectAt(unsigned index, JSObject * outJSObject);

    const char * getNameAt(unsigned index) {
        assert(object != NULL);

        return json_object_get_name(object, index);
    }

    unsigned getCount() {
        return object ? (unsigned)json_object_get_count(object) : 0;
    }

    bool hasProperty(const char * name) {
        return object ? json_object_has_value(object, name) == 1 : false;
    }

    bool getObjectByName(const char * name, JSObject * outJSObject);

    const char * getStringByName(const char * name) {
        assert(object != NULL);

        const char * text = json_object_get_string(object, name);
        if (text == NULL) {
            return NULL; // let consumer file the log
        }

        return text;
    }

    double getNumberByName(const char * name) {
        assert(object != NULL);
        // API returns 0.0 on fail hence it doesn't actually have a good
        // fail discovery strategy
        return json_object_get_number(object, name);
    }
};

unsigned urldecode(const char * url, unsigned length, AutoString * outURL);
bool SyncTimeToNTP();

#endif /* INC_UTILITY_H */