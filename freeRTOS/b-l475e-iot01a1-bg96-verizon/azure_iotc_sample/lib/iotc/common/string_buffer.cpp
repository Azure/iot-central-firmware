// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "iotc_internal.h"

static char convertToHex(char ch) {
    static const char * lst = "0123456789ABCDEF";
    return *(lst + (ch & 15));
}

static char convertFromHex(char ch) {
    assert(isdigit(ch) || isalpha(ch)); // isalnum ?
    if (ch <= '9') {
       return ch - '0';
    } else {
        if (ch <= 'Z') {
            ch = ch - 'A';
        } else {
            ch = ch - 'a';
        }

        return ch + 10;
    }
}

bool StringBuffer::startsWith(const char* str, size_t len) {
    if (len > length) return false;

    const char *buffer = data == NULL ? immutable : data;
    assert(buffer != NULL);

    for (size_t i = 0; i < len; i++) {
        if (str[i] != buffer[i]) return false;
    }
    return true;
}

int32_t StringBuffer::indexOf(const char* look_for, size_t look_for_length,
  int32_t start_index) {

    const char *buffer = data == NULL ? immutable : data;
    assert(buffer != NULL);

    if (look_for_length > length) {
        return -1;
    }

    for (size_t pos = start_index; pos < length; pos++) {
        if (length - pos < look_for_length) {
            return -1;
        }

        if (buffer[pos] == *look_for) {
            size_t sub = 1;
            for (; sub < look_for_length; sub++) {
                if (buffer[pos + sub] != look_for[sub]) break;
            }

            if (sub == look_for_length) {
                return pos;
            }
        }
    }

    return -1;
}

bool StringBuffer::urlEncode() {
    assert(data != NULL);
    size_t buffer_length = (length * 3) + 1;
    char *buffer = (char*) IOTC_MALLOC(buffer_length);
    if (buffer == NULL) {
        return false;
    }

    char *tmp = buffer;
    assert(buffer != NULL);

    for (size_t i = 0; i < length; i++) {
        char ch = data[i];
        if (isalnum(ch) ||
            ch == '_' || ch == '-' || ch == '~' || ch == '.') {
          *tmp = ch;
        } else if (ch == ' ') {
          *tmp = '+';
        } else {
            *tmp++ = '%';
            *tmp++ = convertToHex(ch >> 4);
            *tmp   = convertToHex(ch & 15);
        }
        tmp++;
    }
    *tmp = 0;

    clear(); // free prev memory

    data = buffer;
    length = (size_t)tmp - (size_t)buffer;
    return true;
}

bool StringBuffer::urlDecode() { // in-memory
    assert(data != NULL);
    char *tmp = data; // fast

    for (size_t i = 0; i < length; i++) {
        char ch = data[i];
        if (ch == '%') {
            if (i + 2 < length) {
                *tmp = convertFromHex(data[i + 1]) << 4 |
                       convertFromHex(data[i + 2]);
                i += 2;
            }
        } else if (ch == '+') {
            *tmp = ' ';
        } else {
            *tmp = ch;
        }
        tmp++;
    }
    *tmp = 0;
    length = (size_t)tmp - (size_t)data;

    return true;
}

bool StringBuffer::hash(const char *key_, unsigned key_length)
{
    assert(data != NULL);
    mbedtls_md_type_t md = MBEDTLS_MD_SHA256;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t ctx;

    md_info = mbedtls_md_info_from_type(md);
    unsigned hash_size = (unsigned) mbedtls_md_get_size(md_info);
    unsigned char *hmac_hash = (unsigned char*) IOTC_MALLOC(hash_size + 1);
    memset(hmac_hash, 0, hash_size + 1);

    mbedtls_md_init(&ctx);
    int res = 0;
#define LOG_FAIL__(X) if ((res=X)!=0) { IOTC_LOG("Error: StringBuffer::hash %s has failed with %d return value.", TO_STRING(X), res); return false;}
    LOG_FAIL__(mbedtls_md_setup(&ctx, md_info, 1));
    LOG_FAIL__(mbedtls_md_hmac_starts(&ctx, (const unsigned char*) key_, key_length));
    LOG_FAIL__(mbedtls_md_hmac_update(&ctx, (const unsigned char*) data, length));
    LOG_FAIL__(mbedtls_md_hmac_finish(&ctx, hmac_hash));
#undef LOG_FAIL__

    IOTC_FREE(data);
    data = (char*)hmac_hash;
    setLength(hash_size);
    mbedtls_md_free(&ctx);

    return true;
}

bool StringBuffer::base64Decode() {
    assert(data != NULL && length > 0);
    char *decoded = (char*) IOTC_MALLOC(length + 1); assert(decoded != NULL);
    size_t keyLength = 0;
    mbedtls_base64_decode((unsigned char*)decoded, length, &keyLength,
        (const unsigned char*)data, getLength());
    assert(keyLength > 0);
    IOTC_FREE(data);
    data = (char*) IOTC_MALLOC(keyLength + 1); assert(data != NULL);
    memcpy(data, decoded, keyLength);
    setLength(keyLength);
    IOTC_FREE(decoded);
    return true;
}

bool StringBuffer::base64Encode() {
    assert(data != NULL && length > 0);
    char *decoded = (char*) IOTC_MALLOC(length * 3); assert(decoded != NULL);
    size_t keyLength = 0;
    mbedtls_base64_encode((unsigned char*)decoded, length * 3, &keyLength,
        (const unsigned char*)data, getLength());
    assert(keyLength > 0);
    IOTC_FREE(data);
    data = (char*) IOTC_MALLOC(keyLength + 1); assert(data != NULL);
    memcpy(data, decoded, keyLength);
    setLength(keyLength);
    IOTC_FREE(decoded);
    return true;
}

StringBuffer::StringBuffer(StringBuffer &buffer): data(NULL), immutable(NULL) {
    length = 0;

    initialize(buffer.data, buffer.length);
}

StringBuffer::StringBuffer(const char * str, unsigned int lengthStr, bool isCopy):
    data(NULL), immutable(NULL) {
    if (isCopy) {
        length = 0;
        if (str != NULL) {
            initialize(str, lengthStr);
        }
    } else {
        assert(str != NULL);
        immutable = str;
        length = lengthStr;
    }
}

StringBuffer::StringBuffer(unsigned lengthStr): data(NULL), immutable(NULL) {
    length = 0;

    alloc(lengthStr + 1);
    assert(data != NULL); // out of memory?

    length = lengthStr;
}

void StringBuffer::initialize(const char * str, unsigned lengthStr) {
    if (str != NULL) {
        alloc(lengthStr + 1); // +1 for \0
        assert(data != NULL && immutable == NULL); // out of memory?

        memcpy(data, str, lengthStr);
        data[lengthStr] = char(0);
        length = lengthStr;
    }
}

void StringBuffer::alloc(unsigned lengthStr) {
    ASSERT_OR_FAIL_FAST(lengthStr != 0 && data == NULL && immutable == NULL);

    data = (char*) IOTC_MALLOC(lengthStr);

    ASSERT_OR_FAIL_FAST(data != NULL);
    memset(data, 0, lengthStr);
}

void StringBuffer::set(unsigned index, char c) {
    assert(index < length && data != NULL);
    data[index] = c;
}

void StringBuffer::clear() {
    if (data != NULL) {
        IOTC_FREE(data);
        data = NULL;
    }
}

StringBuffer::~StringBuffer() {
    this->clear();
}

void StringBuffer::setLength(unsigned l) {
    ASSERT_OR_FAIL_FAST(data != NULL);
    length = l;
    data[l] = char(0);
}
