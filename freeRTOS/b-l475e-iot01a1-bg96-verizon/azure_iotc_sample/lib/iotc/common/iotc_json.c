#include <stdlib.h>
#include "iotc_json.h"

int jsobject_initialize(jsobject_t *object, const char *js, unsigned len) {
  object->json = js;
  object->length = len;
  object->tokens = NULL;
  jsmn_init(&object->parser);

  object->tokenCount = jsmn_parse(&object->parser, object->json, object->length, NULL, 0);
  if (object->tokenCount < 0) {
    return object->tokenCount;
  }

  jsmn_init(&object->parser);
  object->tokens = (jsmntok_t *)IOTC_MALLOC(object->tokenCount * sizeof(jsmntok_t));
  jsmn_parse(&object->parser, object->json, object->length, object->tokens, object->tokenCount);
  return 0;
}

int jsobject_compare(jsobject_t *object, int index, const char *s) {
  if (index + 1 >= object->tokenCount) {
    return -1;
  }

  jsmntok_t token = object->tokens[index + 1];
  if ((int)strlen(s) == token.end - token.start &&
      strncmp(object->json + token.start, s, token.end - token.start) == 0) {
    return 0;
  }
  return -1;
}

// caller responsible from free'ing the memory
char *jsobject_get_name_at(jsobject_t *object, int index) {
  index *= 2;  // even indexes are names, odd indexes are values
  return jsobject_get_string_at(object, index);
}

// caller responsible from free'ing the memory
char *jsobject_get_string_at(jsobject_t *object, int index) {
  if (index + 1 >= object->tokenCount) {
    return NULL;
  }

  jsmntok_t token = object->tokens[index + 1];
  unsigned n = token.end - token.start;
  char *name = (char *)IOTC_MALLOC(1 + n);
  memcpy(name, object->json + token.start, n);
  name[n] = 0;
  return name;
}

unsigned jsobject_get_count(jsobject_t *object) { return object->tokenCount; }

int jsobject_get_index_by_name(jsobject_t *object, const char *name) {
  for (int i = 0; i < object->tokenCount; i++) {
    if (jsobject_compare(object, i, name) == 0) return i + 1;
  }
  return -1;
}

void jsobject_free(jsobject_t *object) {
  if (object->tokens != NULL) {
    IOTC_FREE(object->tokens);
    object->tokens = NULL;
  }
}

int jsobject_get_object_by_name(jsobject_t *object, const char *name,
                                jsobject_t *out) {
  int index = jsobject_get_index_by_name(object, name);
  if (index == -1) {
    return 1;  // let consumer file the log
  }

  jsmntok_t token = object->tokens[index + 1];
  unsigned n = token.end - token.start;
  jsobject_initialize(out, object->json + token.start, n);
  return 0;
}

char *jsobject_get_string_by_name(jsobject_t *object, const char *name) {
  int index = jsobject_get_index_by_name(object, name);
  if (index == -1) {
    return NULL;  // let consumer file the log
  }

  jsmntok_t token = object->tokens[index + 1];
  unsigned n = token.end - token.start;
  char *value = (char *)IOTC_MALLOC(1 + n);
  memcpy(value, object->json + token.start, n);
  value[n] = 0;
  return value;
}

double jsobject_get_number_by_name(jsobject_t *object, const char *name) {
  char *str = jsobject_get_string_by_name(object, name);
  if (str == NULL) return 0;

  double n = atof(str);
  IOTC_FREE(str);
  return n;
}

// caller responsible from free'ing the memory
char *jsobject_get_data_by_name(jsobject_t *object, const char*name) {
  int index = jsobject_get_index_by_name(object, name);
  if (index == -1) {
    return NULL;  // let consumer file the log
  }

  jsmntok_t token = object->tokens[index + 1];
  unsigned n = (token.end - token.start) + (token.type == JSMN_STRING ? 2 : 0);
  unsigned start = token.type == JSMN_STRING ? (object->json + token.start - 1) : (object->json + token.start);
  char *value = (char *)IOTC_MALLOC(1 + n);
  memcpy(value, start, n);
  value[n] = 0;
  return value;
}