// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "iotc_internal.h"
#include "../common/iotc_json.h"

IOTLogLevel gLogLevel = IOTC_LOGGING_DISABLED;

void setLogLevel(IOTLogLevel l) { gLogLevel = l; }
IOTLogLevel getLogLevel() { return gLogLevel; }

static unsigned EXPIRES = 21600;
void setEXPIRES(unsigned e) { EXPIRES = e; }

unsigned strlen_s_(const char *str, int max_expected) {
  int ret_val = 0;
  while (*(str++) != 0) {
    ret_val++;
    if (ret_val >= max_expected) return max_expected;
  }

  return ret_val;
}

void vMainUARTPrintString(char *pcString);
void vLoggingPrintf(const char *pcFormat, ...) {
  size_t len = 0;
  va_list args;

  StringBuffer buffer(STRING_BUFFER_512);
  va_start(args, pcFormat);

  len = vsnprintf(*buffer, STRING_BUFFER_512, pcFormat, args);
  if (len < 0) {
    return;
  }
  buffer.setLength(len);
  va_end(args);

  vMainUARTPrintString(*buffer);
}

unsigned long getNow();

// Self MQTT option supports singletonContext only.
static IOTContextInternal *singletonContext = NULL;

IOTContextInternal *getSingletonContext() { return singletonContext; }
void setSingletonContext(IOTContextInternal *ctx) { singletonContext = ctx; }

int getUsernameAndPasswordFromConnectionString(const char *connectionString,
                                               size_t connectionStringLength,
                                               StringBuffer &hostName,
                                               StringBuffer &deviceId,
                                               StringBuffer &username,
                                               StringBuffer &password) {
  // TODO: improve this so we don't depend on a particular order in connection
  // string
  StringBuffer connStr(connectionString, connectionStringLength);

  int32_t hostIndex = connStr.indexOf(HOSTNAME_STRING, HOSTNAME_LENGTH);
  size_t length = connStr.getLength();
  if (hostIndex != 0) {
    IOTC_LOG(
        F("ERROR: connectionString doesn't start with HostName=  RESULT:%d"),
        hostIndex);
    return 1;
  }

  int32_t deviceIndex = connStr.indexOf(DEVICEID_STRING, DEVICEID_LENGTH);
  if (deviceIndex == -1) {
    IOTC_LOG(F("ERROR: ;DeviceId= not found in the connectionString"));
    return 1;
  }

  int32_t keyIndex = connStr.indexOf(KEY_STRING, KEY_LENGTH);
  if (keyIndex == -1) {
    IOTC_LOG(F("ERROR: ;SharedAccessKey= not found in the connectionString"));
    return 1;
  }

  hostName.initialize(connectionString + HOSTNAME_LENGTH,
                      deviceIndex - HOSTNAME_LENGTH);
  deviceId.initialize(connectionString + (deviceIndex + DEVICEID_LENGTH),
                      keyIndex - (deviceIndex + DEVICEID_LENGTH));

  StringBuffer keyBuffer(length - (keyIndex + KEY_LENGTH));
  memcpy(*keyBuffer, connectionString + (keyIndex + KEY_LENGTH),
         keyBuffer.getLength());

  StringBuffer hostURLEncoded(hostName);
  hostURLEncoded.urlEncode();

  size_t expires = getNow() + EXPIRES;

  StringBuffer stringToSign(hostURLEncoded.getLength() + STRING_BUFFER_128);
  StringBuffer deviceIdEncoded(deviceId);
  deviceIdEncoded.urlEncode();
  size_t keyLength =
      snprintf(*stringToSign, stringToSign.getLength(), "%s%s%s\n%lu000",
               *hostURLEncoded, "%2Fdevices%2F", *deviceIdEncoded, expires);
  stringToSign.setLength(keyLength);

  keyBuffer.base64Decode();
  stringToSign.hash(*keyBuffer, keyBuffer.getLength());
  if (!stringToSign.base64Encode() || !stringToSign.urlEncode()) {
    IOTC_LOG(F("ERROR: stringToSign base64Encode / urlEncode has failed."));
    return 1;
  }

  StringBuffer passwordBuffer(STRING_BUFFER_512);
  size_t passLength = snprintf(
      *passwordBuffer, STRING_BUFFER_512,
      "SharedAccessSignature sr=%s%s%s&sig=%s&se=%ld000", *hostURLEncoded,
      "%2Fdevices%2F", *deviceIdEncoded, *stringToSign, expires);

  assert(passLength && passLength < STRING_BUFFER_512);
  passwordBuffer.setLength(passLength);

  password.initialize(*passwordBuffer, passwordBuffer.getLength());

  const char *usernameTemplate = "%s/%s/api-version=2016-11-14";
  StringBuffer usernameBuffer((strlen(usernameTemplate) - 3 /* %s twice */) +
                              hostName.getLength() + deviceId.getLength());

  size_t expLength = snprintf(*usernameBuffer, usernameBuffer.getLength(),
                              usernameTemplate, *hostName, *deviceId);
  assert(expLength <= usernameBuffer.getLength());

  username.initialize(*usernameBuffer, usernameBuffer.getLength());

  IOTC_LOG(F("\r\n"
             "hostname: %s\r\n"
             "deviceId: %s\r\n"
             "username: %s\r\n"
             "password: %s\r\n"),
           *hostName, *deviceId, *username, *password);

  return 0;
}

int getDPSAuthString(const char *scopeId, const char *deviceId, const char *key,
                     char *buffer, int bufferSize, size_t &outLength) {
  size_t expires = getNow() + EXPIRES;

  StringBuffer deviceIdEncoded(deviceId, strlen(deviceId));
  deviceIdEncoded.urlEncode();

  StringBuffer stringToSign(STRING_BUFFER_256);
  size_t size =
      snprintf(*stringToSign, STRING_BUFFER_256, "%s%%2Fregistrations%%2F%s",
               scopeId, *deviceIdEncoded);
  assert(size < STRING_BUFFER_256);
  stringToSign.setLength(size);

  StringBuffer sr(stringToSign);
  size = snprintf(*stringToSign, STRING_BUFFER_256, "%s\n%lu000", *sr, expires);
  assert(size < STRING_BUFFER_256);
  stringToSign.setLength(size);

  size = 0;
  StringBuffer keyDecoded(key, strlen(key));
  // IOTC_LOG("KEY: %s SIGN: %s", key, *stringToSign);
  keyDecoded.base64Decode();
  stringToSign.hash(*keyDecoded, keyDecoded.getLength());
  if (!stringToSign.base64Encode() || !stringToSign.urlEncode()) {
    IOTC_LOG(F("ERROR: stringToSign base64Encode / urlEncode has failed."));
    return 1;
  }
  // IOTC_LOG("SIGNED: %s", *stringToSign);

  outLength = snprintf(buffer, bufferSize,
                       "authorization: SharedAccessSignature "
                       "sr=%s&sig=%s&se=%lu000&skn=registration",
                       *sr, *stringToSign, expires);
  assert(outLength > 0 && outLength < bufferSize);
  buffer[outLength] = 0;

  return 0;
}

// send telemetry etc. confirmation callback
/* MessageSent */
void sendConfirmationCallback(const char *buffer, size_t size) {
  IOTContextInternal *internal = (IOTContextInternal *)singletonContext;
  int result = 0;

  if (internal->callbacks[/*IOTCallbacks::*/ MessageSent].callback) {
    IOTCallbackInfo info;
    info.eventName = "MessageSent";
    info.tag = NULL;
    info.payload = (const char *)buffer;
    info.payloadLength = (unsigned)size;
    info.appContext =
        internal->callbacks[/*IOTCallbacks::*/ ::MessageSent].appContext;
    info.statusCode = (int)result;
    info.callbackResponse = NULL;
    internal->callbacks[/*IOTCallbacks::*/ ::MessageSent].callback(internal,
                                                                   &info);
  }
}

/* Command */
static int onCommand(const char *method_name, const char *payload, size_t size,
                     char **response, size_t *resp_size,
                     void *userContextCallback) {
  assert(response != NULL && resp_size != NULL);
  *response = NULL;
  *resp_size = 0;
  IOTContextInternal *internal = (IOTContextInternal *)userContextCallback;
  assert(internal != NULL);

  if (internal->callbacks[/*IOTCallbacks::*/ ::Command].callback) {
    IOTCallbackInfo info;
    info.eventName = "Command";
    info.tag = method_name;
    info.payload = (const char *)payload;
    info.payloadLength = (unsigned)size;
    info.appContext =
        internal->callbacks[/*IOTCallbacks::*/ ::Command].appContext;
    info.statusCode = 0;
    info.callbackResponse = NULL;
    internal->callbacks[/*IOTCallbacks::*/ ::Command].callback(internal, &info);

    if (info.callbackResponse != NULL) {
      *response = (char *)info.callbackResponse;
      *resp_size = strlen((char *)info.callbackResponse);
    }
    return 200;
  }

  return 500;
}

/* ConnectionStatus */
void connectionStatusCallback(IOTConnectionState status,
                              IOTContextInternal *internal) {
  if (internal->callbacks[/*IOTCallbacks::*/ ::ConnectionStatus].callback) {
    IOTCallbackInfo info;
    info.eventName = "ConnectionStatus";
    info.tag = NULL;
    info.payload = NULL;
    info.payloadLength = 0;
    info.appContext =
        internal->callbacks[/*IOTCallbacks::*/ ::ConnectionStatus].appContext;
    info.statusCode = status;
    info.callbackResponse = NULL;
    internal->callbacks[/*IOTCallbacks::*/ ::ConnectionStatus].callback(
        internal, &info);
  }
}

void sendOnError(IOTContextInternal *internal, const char *message) {
  if (internal->callbacks[/*IOTCallbacks::*/ ::Error].callback) {
    IOTCallbackInfo info;
    info.eventName = "Error";
    info.tag = message;  // message lifetime should be managed by us
    info.payload = NULL;
    info.payloadLength = 0;
    info.appContext =
        internal->callbacks[/*IOTCallbacks::*/ ::Error].appContext;
    info.statusCode = 1;
    info.callbackResponse = NULL;
    internal->callbacks[/*IOTCallbacks::*/ ::Error].callback(internal, &info);
  }
}

void echoDesired(IOTContextInternal *internal, const char *propertyName,
                 StringBuffer &message, const char *status, int statusCode) {
  jsobject_t rootObject;
  jsobject_initialize(&rootObject, *message, message.getLength());
  jsobject_t propertyNameObject;

  if (jsobject_get_object_by_name(&rootObject, propertyName,
                                  &propertyNameObject) != 0) {
    IOTC_LOG(
        "ERROR: echoDesired has failed due to payload doesn't include the "
        "property => %s",
        *message);
    jsobject_free(&rootObject);
    return;
  }

  char *value = jsobject_get_data_by_name(&propertyNameObject, "value");
  double desiredVersion = jsobject_get_number_by_name(&rootObject, "$version");

  const char *echoTemplate =
      "{\"%s\":{\"value\":%s,\"statusCode\":%d,\
\"status\":\"%s\",\"desiredVersion\":%d}}";
  uint32_t buffer_size = strlen(echoTemplate) + strlen(value) +
                         3 /* statusCode */
                         + 32 /* status */ + 23 /* version max */;
  buffer_size = iotc_min(buffer_size, 512);

  const char *topicName = "$iothub/twin/PATCH/properties/reported/?$rid=%d";
  DoNode *node = (DoNode *)IOTC_MALLOC(sizeof(DoNode));
  memset(node, 0, sizeof(DoNode));

  node->topic.alloc(strlen(topicName) + 21);
  node->topic.setLength(snprintf(*node->topic, strlen(topicName) + 21,
                                 topicName, internal->messageId++));
  node->data.alloc(buffer_size);
  node->data.setLength(snprintf(*node->data, buffer_size, echoTemplate,
                                propertyName, value, statusCode, status,
                                (int)desiredVersion));

  IOTC_FREE(value);

  assert(getSingletonContext()->DoNodeMutex != NULL);
  xSemaphoreTake(getSingletonContext()->DoNodeMutex, portMAX_DELAY);
  if (getSingletonContext()->DoNodeList == NULL) {
    getSingletonContext()->DoNodeList = node;
  } else {
    DoNode *root = getSingletonContext()->DoNodeList;
    getSingletonContext()->DoNodeList = node;
    node->next = root;
  }
  xSemaphoreGive(getSingletonContext()->DoNodeMutex);

  jsobject_free(&propertyNameObject);
  jsobject_free(&rootObject);
}

void callDesiredCallback(IOTContextInternal *internal, const char *propertyName,
                         StringBuffer &payload) {
  const char *response = "completed";
  if (internal->callbacks[/*IOTCallbacks::*/ ::SettingsUpdated].callback) {
    IOTCallbackInfo info;
    info.eventName = "SettingsUpdated";
    info.tag = propertyName;
    info.payload = *payload;
    info.payloadLength = payload.getLength();
    info.appContext =
        internal->callbacks[/*IOTCallbacks::*/ ::SettingsUpdated].appContext;
    info.statusCode = 200;
    info.callbackResponse = NULL;
    internal->callbacks[/*IOTCallbacks::*/ ::SettingsUpdated].callback(internal,
                                                                       &info);
    if (info.callbackResponse) {
      response = (const char *)info.callbackResponse;
    }
    echoDesired(internal, propertyName, payload, response, info.statusCode);
  }
}

static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state,
                                       StringBuffer &payload,
                                       void *userContextCallback) {
  IOTContextInternal *internal = (IOTContextInternal *)userContextCallback;
  assert(internal != NULL);

  jsobject_t desired;
  jsobject_initialize(&desired, *payload, payload.getLength());

  for (unsigned i = 0, count = jsobject_get_count(&desired); i < count;
       i += 2) {
    char *itemName = jsobject_get_name_at(&desired, i);
    if (itemName != NULL && itemName[0] != '$') {
      callDesiredCallback(internal, itemName, payload);
    }
    if (itemName) IOTC_FREE(itemName);
  }
  jsobject_free(&desired);
}

void sendDoNodes() {
  if (getSingletonContext()->DoNodeMutex == NULL) {
    getSingletonContext()->DoNodeMutex = xSemaphoreCreateMutex();
  }

  xSemaphoreTake(getSingletonContext()->DoNodeMutex, portMAX_DELAY);
  while (getSingletonContext()->DoNodeList != NULL) {
    DoNode *root = getSingletonContext()->DoNodeList;
    getSingletonContext()->DoNodeList = getSingletonContext()->DoNodeList->next;

    if (mqtt_publish(getSingletonContext(), *root->topic,
                     root->topic.getLength(), *root->data,
                     root->data.getLength()) != 0) {
      IOTC_LOG(F("ERROR: mqtt_publish has failed during C2D with response "
                 "topic '%s' and response '%s'"),
               *root->topic, *root->data);
    }
    sendConfirmationCallback(*root->data, root->data.getLength());
    IOTC_FREE(root);
  }
  xSemaphoreGive(getSingletonContext()->DoNodeMutex);
}

void handlePayload(char *msg, unsigned long msg_length, char *topic,
                   unsigned long topic_length) {
  if (topic_length) {
    assert(topic != NULL);
    StringBuffer topicName(topic, topic_length);
    if (topicName.startsWith("$iothub/twin/res", strlen("$iothub/twin/res")))
      return;

    StringBuffer payload;
    if (msg_length) {
      payload.initialize(msg, msg_length);
    }

    if (topicName.startsWith(
            "$iothub/twin/PATCH/properties/desired/",
            strlen("$iothub/twin/PATCH/properties/desired/"))) {
      deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_ALL, payload,
                                 singletonContext);
    } else if (topicName.startsWith("$iothub/methods",
                                    strlen("$iothub/methods"))) {
      int index = topicName.indexOf("$rid=", 5, 0);
      if (index == -1) {
        IOTC_LOG(F("ERROR: corrupt C2D message topic => %s"), *topicName);
        return;
      }

      const char *topicId = (*topicName + index + 5);
      const char *topicTemplate = "$iothub/methods/POST/";
      const int topicTemplateLength = strlen(topicTemplate);
      index = topicName.indexOf("/", 1, topicTemplateLength + 1);
      if (index == -1) {
        IOTC_LOG(F("ERROR: corrupt C2D message topic (methodName) => %s"),
                 *topicName);
        return;
      }

      StringBuffer methodName(topic + topicTemplateLength,
                              index - topicTemplateLength);

      const char *constResponse = "{}";
      char *response = NULL;
      size_t respSize = 0;
      int rc = onCommand(*methodName, msg, msg_length, &response, &respSize,
                         getSingletonContext());
      if (respSize == 0) {
        respSize = 2;
      } else {
        constResponse = response;
      }

      DoNode *node = (DoNode *)IOTC_MALLOC(sizeof(DoNode));
      memset(node, 0, sizeof(DoNode));

      node->topic.alloc(STRING_BUFFER_128);
      node->topic.setLength(snprintf(*node->topic, STRING_BUFFER_128,
                                     "$iothub/methods/res/%d/?$rid=%s", rc,
                                     topicId));
      node->data.alloc(respSize);
      memcpy(*node->data, constResponse, respSize);
      node->data.setLength(respSize);

      assert(getSingletonContext()->DoNodeMutex != NULL);
      xSemaphoreTake(getSingletonContext()->DoNodeMutex, portMAX_DELAY);
      if (getSingletonContext()->DoNodeList == NULL) {
        getSingletonContext()->DoNodeList = node;
      } else {
        DoNode *root = getSingletonContext()->DoNodeList;
        getSingletonContext()->DoNodeList = node;
        node->next = root;
      }
      xSemaphoreGive(getSingletonContext()->DoNodeMutex);

      if (response != constResponse) {
        IOTC_FREE(response);
      }
    } else {
      IOTC_LOG(F("ERROR: unknown twin topic: %s, msg: %s"), topic,
               msg_length ? msg : "NULL");
    }
  }
}

/* extern */
int iotc_send_telemetry(IOTContext ctx, const char *payload, unsigned length) {
  return iotc_send_telemetry_with_system_properties(ctx, payload, length, NULL,
                                                    0);
}

int iotc_send_telemetry_with_system_properties(IOTContext ctx,
                                               const char *payload,
                                               unsigned length,
                                               const char *sysPropPayload,
                                               unsigned sysPropPayloadLength) {
  CHECK_NOT_NULL(ctx)
  CHECK_NOT_NULL(payload)

  IOTContextInternal *internal = (IOTContextInternal *)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  if ((sysPropPayload == NULL && sysPropPayloadLength != 0) ||
      (sysPropPayload != NULL && sysPropPayloadLength == 0)) {
    IOTC_LOG(
        "ERROR: (iotc_send_telemetry_with_system_properties) sysPropPayload "
        "doesn't match with sysPropPayloadLength");
    return 1;
  }

  StringBuffer topic(internal->deviceId.getLength() +
                               strlen("devices/ /messages/events/") +
                               sysPropPayloadLength);
  if (sysPropPayloadLength > 0) {
    topic.setLength(
        snprintf(*topic, topic.getLength(), "devices/%s/messages/events/%.*s",
                 *internal->deviceId, sysPropPayloadLength, sysPropPayload));
  } else {
    topic.setLength(snprintf(*topic, topic.getLength(),
                             "devices/%s/messages/events/",
                             *internal->deviceId));
  }

  if (mqtt_publish(internal, *topic, topic.getLength(), payload, length) != 0) {
    IOTC_LOG("ERROR: (iotc_send_telemetry) MQTTClient publish has failed => %s",
             payload);
    return 1;
  }

  sendConfirmationCallback(payload, length);
  return 0;
}

/* extern */
int iotc_send_property(IOTContext ctx, const char *payload, unsigned length) {
  CHECK_NOT_NULL(ctx)
  CHECK_NOT_NULL(payload)

  IOTContextInternal *internal = (IOTContextInternal *)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  const char *topicName = "$iothub/twin/PATCH/properties/reported/?$rid=%d";
  StringBuffer topic(strlen(topicName) +
                     21);  // + 2 for %d == +23 in case requestId++ overflows
  topic.setLength(
      snprintf(*topic, topic.getLength(), topicName, internal->messageId++));

  if (mqtt_publish(internal, *topic, topic.getLength(), payload, length) != 0) {
    IOTC_LOG(F("ERROR: (iotc_send_property) MQTTClient publish has failed."));
    return 1;
  }

  sendConfirmationCallback(payload, length);
  return 0;
}

/* extern */
int iotc_init_context(IOTContext *ctx) {
  CHECK_NOT_NULL(ctx)

  if (getSingletonContext() != NULL) {
    IOTC_LOG(
        F("ERROR: (iotc_init_context) Self MQTT version supports singleton "
          "context only."));
    return 1;
  }

  MUST_CALL_BEFORE_INIT((*ctx));
  IOTContextInternal *internal =
      (IOTContextInternal *)IOTC_MALLOC(sizeof(IOTContextInternal));
  CHECK_NOT_NULL(internal);
  memset(internal, 0, sizeof(IOTContextInternal));
  *ctx = (void *)internal;

  setSingletonContext(internal);
  sendDoNodes();

  return 0;
}
