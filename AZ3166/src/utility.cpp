// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/utility.h"

#include "SystemWiFi.h"
#include "NTPClient.h"

typedef union json_value_value {
    char        *string;
    double       number;
    JSON_Object *object;
    JSON_Array  *array;
    int          boolean;
    int          null;
} JSON_Value_Value;

struct json_object_t {
    JSON_Value  *wrapping_value;
    char       **names;
    JSON_Value **values;
    size_t       count;
    size_t       capacity;
};

struct json_value_t {
    JSON_Value      *parent;
    JSON_Value_Type  type;
    JSON_Value_Value value;
};

// detect if parson data types have changed.
static_assert(sizeof(JSON_Value) == sizeof(json_value_t), "parson data types have changed?");
static_assert(sizeof(JSON_Object) == sizeof(json_object_t), "parson data types have changed?");

// unchanged. redef
typedef struct json_object_t JSON_Object;
typedef struct json_value_t JSON_Value;

const char * JSObject::toString() {
    if (object->wrapping_value == NULL) return NULL;
    return json_value_get_string(object->wrapping_value /* hacky */);
}

JSObject::~JSObject() {
    if (value != NULL) {
        memset((void*)value, 0, sizeof(JSON_Value));
        json_value_free(value);
        value = NULL;
    }
}

bool JSObject::getObjectAt(unsigned index, JSObject * outJSObject) {
    if (index >= getCount()) return false;

    outJSObject->value = (JSON_Value*) malloc(sizeof(JSON_Value)); // memory will be cleaned up at outJSObject's de-cons.
    memcpy(outJSObject->value, value, sizeof(JSON_Value));
    outJSObject->toObject();

    JSON_Value * subValue =  json_object_get_value_at(outJSObject->object, index);
    if (subValue == NULL) return false;

    outJSObject->object = json_value_get_object(subValue);
    if (outJSObject->object == NULL) return false;

    return true;
}

bool JSObject::getObjectByName(const char * name, JSObject * outJSObject) {
    outJSObject->value = (JSON_Value*) malloc(sizeof(JSON_Value));
    memcpy(outJSObject->value, value, sizeof(JSON_Value));
    outJSObject->toObject();

    JSON_Object* subObject = json_object_get_object(outJSObject->object, name);
    if (subObject == NULL) {
        // outJSObject->value memory freed by it's own de-constructor.
        return false; // let consumer file the log
    }


    outJSObject->object = subObject;
    return true;
}

// As there is a problem of sprintf %f in Arduino, follow https://github.com/blynkkk/blynk-library/issues/14 to implement dtostrf
char * dtostrf(double number, signed char width, unsigned char prec, char *s) {
    if(isnan(number)) {
        strcpy(s, "nan");
        return s;
    }
    if(isinf(number)) {
        strcpy(s, "inf");
        return s;
    }

    if(number > 4294967040.0 || number < -4294967040.0) {
        strcpy(s, "ovf");
        return s;
    }
    char* out = s;
    // Handle negative numbers
    if(number < 0.0) {
        *out = '-';
        ++out;
        number = -number;
    }
    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for(uint8_t i = 0; i < prec; ++i) {
        rounding /= 10.0;
    }
    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long) number;
    double remainder = number - (double) int_part;
    out += sprintf(out, "%d", int_part);

    // Print the decimal point, but only if there are digits beyond
    if(prec > 0) {
        *out = '.';
        ++out;
    }

    while(prec-- > 0) {
        remainder *= 10.0;
        if((int)remainder == 0){
            *out = '0';
            ++out;
        }
    }
    sprintf(out, "%d", (int) remainder);
    return s;
}

unsigned char h2int(char c) {
    if (c >= '0' && c <='9') {
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f') {
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F') {
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

unsigned urldecode(const char * url, unsigned length, AutoString * outURL) {
    assert(outURL != NULL);
    unsigned resultLength = 0;

    if (length == 0) {
        return resultLength;
    }

    outURL->alloc(length);
    for (unsigned i = 0; i < length; i++) {
        char c = *(url + i);
        if (c == '+') {
            outURL->set(resultLength++, ' ');
        } else {
            if (c == '%') {
                const char code0 = *(url + (++i));
                const char code1 = *(url + (++i));
                c = (h2int(code0) << 4) | h2int(code1);
            }
            outURL->set(resultLength++, c);
        }
    }
    outURL->set(resultLength, 0);

    return resultLength;
}

bool SyncTimeToNTP() {
    static const char* ntpHost[] =
    {
        "pool.ntp.org",
        "cn.pool.ntp.org",
        "europe.pool.ntp.org",
        "asia.pool.ntp.org",
        "oceania.pool.ntp.org"
    };

    for (int i = 0; i < sizeof(ntpHost) / sizeof(ntpHost[0]); i++) {
        NTPClient ntp(WiFiInterface());
        NTPResult res = ntp.setTime((char*)ntpHost[i]);
        if (res == NTP_OK) {
            time_t t = time(NULL);
            (void)Serial.printf("Time from %s, now is (UTC): %s\r\n", ntpHost[i], ctime(&t));
            return true;
        }
    }

    return false;
}