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

char *f2s(float f, int p) {
    char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
    const int iSize = 10;                 // number of bufffers, one for each float before wrapping around
    static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
    static int iCount = 0;                // keep a tab of next place in sBuff to use
    pBuff = sBuff[iCount];                // use this buffer
    if(iCount >= iSize -1){               // check for wrap
        iCount = 0;                         // if wrapping start again and reset
    }
    else{
        iCount++;                           // advance the counter
    }
    return dtostrf(f, 0, p, pBuff);       // call the library function
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

String urldecode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++) {
        c=str.charAt(i);
        if (c == '+') {
            encodedString+=' ';
        } else if (c == '%') {
            i++;
            code0=str.charAt(i);
            i++;
            code1=str.charAt(i);
            c = (h2int(code0) << 4) | h2int(code1);
            encodedString+=c;
        } else {
            encodedString+=c;
        }
    }

    return encodedString;
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