// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOT_COMMON_JSON_H
#define AZURE_IOT_COMMON_JSON_H

#include "jsmn.h"
#include "iotc_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jsobject_t_tag {
  const char *json;
  unsigned length;
  jsmn_parser parser;
  jsmntok_t *tokens;
  int tokenCount;
} jsobject_t;

int jsobject_initialize(jsobject_t *object, const char *js, unsigned len);

int jsobject_compare(jsobject_t *object, int index, const char *s);

// caller responsible from free'ing the memory
char *jsobject_get_name_at(jsobject_t *object, int index);

// caller responsible from free'ing the memory
char *jsobject_get_string_at(jsobject_t *object, int index);

unsigned jsobject_get_count(jsobject_t *object);

int jsobject_get_index_by_name(jsobject_t *object, const char *name);

double jsobject_get_number_by_name(jsobject_t *object, const char *name);

// caller responsible from free'ing the memory
char *jsobject_get_string_by_name(jsobject_t *object, const char *name);

int jsobject_get_object_by_name(jsobject_t *object, const char *name,
                                jsobject_t *out);

// caller responsible from free'ing the memory
char *jsobject_get_data_by_name(jsobject_t *object, const char*name);

void jsobject_free(jsobject_t *object);

#ifdef __cplusplus
}
#endif

#endif  // AZURE_IOT_COMMON_JSON_H
