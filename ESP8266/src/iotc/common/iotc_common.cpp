// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "iotc_internal.h"

void setEXPIRES(unsigned e);

/* extern */
int iotc_on(IOTContext ctx, const char* eventName, IOTCallback callback,
            void* appContext) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL_NOT_EMPTY(eventName, 64);

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_INIT(internal);

#define SETCB_(x, a, b) \
  x.callback = a;       \
  x.appContext = b;
  if (strcmp(eventName, "ConnectionStatus") == 0) {
    SETCB_(internal->callbacks[/*IOTCallbacks::*/ ::ConnectionStatus], callback,
           appContext);
  } else if (strcmp(eventName, "MessageSent") == 0) {
    SETCB_(internal->callbacks[/*IOTCallbacks::*/ ::MessageSent], callback,
           appContext);
  } else if (strcmp(eventName, "Error") == 0) {
    SETCB_(internal->callbacks[/*IOTCallbacks::*/ ::Error], callback,
           appContext);
  } else if (strcmp(eventName, "SettingsUpdated") == 0) {
    SETCB_(internal->callbacks[/*IOTCallbacks::*/ ::SettingsUpdated], callback,
           appContext);
  } else if (strcmp(eventName, "Command") == 0) {
    SETCB_(internal->callbacks[/*IOTCallbacks::*/ ::Command], callback,
           appContext);
  } else {
    IOTC_LOG(F("ERROR: (iotc_on) Unknown event definition. (%s)"), eventName);
    return 1;
  }
#undef SETCB_
  return 0;
}

/* extern */
int iotc_send_state(IOTContext ctx, const char* payload, unsigned length) {
  CHECK_NOT_NULL(ctx)
  CHECK_NOT_NULL(payload)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  return iotc_send_telemetry((IOTContext)internal, payload, length);
}

/* extern */
int iotc_send_event(IOTContext ctx, const char* payload, unsigned length) {
  CHECK_NOT_NULL(ctx)
  CHECK_NOT_NULL(payload)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  return iotc_send_telemetry((IOTContext)internal, payload, length);
}

/* extern */
int iotc_set_global_endpoint(IOTContext ctx, const char* endpoint_uri) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL_NOT_EMPTY(endpoint_uri, 1024);

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_INIT(internal);

  // todo: do not fragment the memory
  if (internal->endpoint != NULL) {
    IOTC_FREE(internal->endpoint);
  }
  internal->endpoint = (char*)IOTC_MALLOC(endpoint_uri_len + 1);
  CHECK_NOT_NULL(internal->endpoint);

  strcpy(internal->endpoint, endpoint_uri);
  *(internal->endpoint + endpoint_uri_len) = 0;
  return 0;
}

/* extern */
int iotc_set_trusted_certs(IOTContext ctx, const char* certs) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL_NOT_EMPTY(certs, 4096);

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  IOTC_LOG(F("ERROR: (iotc_set_trusted_certs) Not implemented."));

  return 0;
}

/* extern */
int iotc_set_proxy(IOTContext ctx, IOTC_HTTP_PROXY_OPTIONS proxy) {
  CHECK_NOT_NULL(ctx)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_INIT(internal);

  IOTC_LOG(F("ERROR: (iotc_set_proxy) Not implemented."));
  return 1;
}

/* extern */
int iotc_set_logging(IOTLogLevel level) {
  if (level < IOTC_LOGGING_DISABLED || level > IOTC_LOGGING_ALL) {
    IOTC_LOG(F("ERROR: (iotc_set_logging) invalid argument. ERROR:0x0001"));
    return 1;
  }
  setLogLevel(level);
  return 0;
}

/* extern */
int iotc_set_model_data(IOTContext ctx, const char* modelData) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL_NOT_EMPTY(modelData, 1024);

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_INIT(internal);

  if (internal->modelData != NULL) return 1;

  internal->modelData = strdup(modelData);
  return internal->modelData != NULL;
}

/* extern */
int iotc_set_token_expiration(IOTContext ctx, unsigned timeout) {
  setEXPIRES(timeout);
  return 0;
}