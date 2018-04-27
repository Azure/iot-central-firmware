// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include "globals.h"
#include <parson/parson.h>

class JSObject
{
private:
    JSON_Value* value;
    JSON_Object* object;

    JSON_Object* toObject() {
        JSON_Object* object = json_value_get_object(value);
        if (object == NULL) {
            LOG_ERROR("JSON value is not an object");
            return NULL;
        }
        return object;
    }
public:
    JSObject(): object(NULL) { }

    JSObject(const char * json_string) {
        value = json_parse_string(json_string);
        if (value == NULL) {
            LOG_ERROR("parsing JSON failed");
        }

        object = toObject();
        if (object == NULL) {
            Serial.printf("json data: %s\r\n", json_string);
        }
    }

    ~JSObject();

    const char * toString();

    bool getObjectAt(unsigned index, JSObject * outJSObject);

    const char * getNameAt(unsigned index) {
        return json_object_get_name(object, index);
    }

    unsigned getCount() {
        return (unsigned)json_object_get_count(object);
    }

    bool hasProperty(const char * name) {
        return json_object_has_value(object, name) == 1;
    }

    bool getObjectByName(const char * name, JSObject * outJSObject);

    const char * getStringByName(const char * name) {
        const char * text = json_object_get_string(object, name);
        if (text == NULL) {
            return NULL; // let consumer file the log
        }

        return text;
    }

    double getNumberByName(const char * name) {
        // API returns 0.0 on fail hence it doesn't actually have a good
        // fail discovery strategy
        return json_object_get_number(object, name);
    }
};

char *f2s(float f, int p);
String urldecode(String str);
bool SyncTimeToNTP();

#endif /* INC_UTILITY_H */