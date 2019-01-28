// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOT_COMMON_JSON_H
#define AZURE_IOT_COMMON_JSON_H

#include "parson.h"
#include "../iotc.h"

namespace AzureIOT
{
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

        const char * getNameAt(unsigned index) {
            if (object == NULL) { LOG_ERROR("(getNameAt) object == NULL!"); return NULL; }

            return json_object_get_name(object, index);
        }

        unsigned getCount() {
            return object ? (unsigned)json_object_get_count(object) : 0;
        }

        bool hasProperty(const char * name) {
            return object ? json_object_has_value(object, name) == 1 : false;
        }

        const char * toString() {
            if (object == NULL) { LOG_ERROR("(toString) object == NULL!"); return NULL; }

            JSON_Value * val = json_object_get_wrapping_value(object);
            if (val == NULL) return NULL;
            return json_value_get_string(val);
        }

        ~JSObject() {
            if (value != NULL && isSubObject == false) {
                json_value_free(value);
                value = NULL;
            }
        }

        bool getObjectAt(unsigned index, JSObject * outJSObject) {
            if (index >= getCount()) return false;

            JSON_Value * subValue =  json_object_get_value_at(object, index);
            if (subValue == NULL) return false;

            outJSObject->isSubObject = true;
            outJSObject->value = subValue;
            outJSObject->toObject();
            if (outJSObject->object == NULL) return false;

            return true;
        }

        bool getObjectByName(const char * name, JSObject * outJSObject) {
            JSON_Object* subObject = json_object_get_object(object, name);
            if (subObject == NULL) {
                // outJSObject->value memory freed by it's own de-constructor.
                return false; // let consumer file the log
            }

            outJSObject->value = json_object_get_wrapping_value(object);
            outJSObject->object = subObject;
            outJSObject->isSubObject = true;
            return true;
        }

        const char * getStringByName(const char * name) {
            if (object == NULL) { LOG_ERROR("(getStringByName) object == NULL!"); return NULL; }

            const char * text = json_object_get_string(object, name);
            if (text == NULL) {
                return NULL; // let consumer file the log
            }

            return text;
        }

        double getNumberByName(const char * name) {
            if (object == NULL) { LOG_ERROR("(getNumberByName) object == NULL!"); return 0; }
            // API returns 0.0 on fail hence it doesn't actually have a good
            // fail discovery strategy
            return json_object_get_number(object, name);
        }
    };
} // namespace AzureIOT

#endif // AZURE_IOT_COMMON_JSON_H
