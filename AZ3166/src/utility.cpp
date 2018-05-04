// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../inc/globals.h"
#include "../inc/utility.h"

#include "SystemWiFi.h"
#include "NTPClient.h"

const char * JSObject::toString() {
    assert(object != NULL);

    JSON_Value * val = json_object_get_wrapping_value(object);
    if (val == NULL) return NULL;
    return json_value_get_string(val);
}

JSObject::~JSObject() {
    if (value != NULL && isSubObject == false) {
        json_value_free(value);
        value = NULL;
    }
}

bool JSObject::getObjectAt(unsigned index, JSObject * outJSObject) {
    if (index >= getCount()) return false;

    JSON_Value * subValue =  json_object_get_value_at(object, index);
    if (subValue == NULL) return false;

    outJSObject->isSubObject = true;
    outJSObject->value = subValue;
    outJSObject->toObject();
    if (outJSObject->object == NULL) return false;

    return true;
}

bool JSObject::getObjectByName(const char * name, JSObject * outJSObject) {
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
            LOG_VERBOSE("Time from %s, now is (UTC): %s", ntpHost[i], ctime(&t));
            return true;
        }
    }

    return false;
}