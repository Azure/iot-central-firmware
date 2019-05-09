// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "../common/iotc_platform.h"
#if defined(PLATFORM_FREERTOS)

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/iotc_internal.h"
#include "../common/iotc_json.h"

unsigned long getNow() {
  return (unsigned long)2200000;  // use ntp utc time to enable this
}

int _getOperationId(IOTContextInternal* internal, const char* dpsEndpoint,
                    const char* scopeId, const char* deviceId,
                    const char* authHeader, char* operationId, char* hostName) {
  int exitCode = 0;
  int tryCount = 0;

  while ((exitCode = iotc_socket_open()) != 0) {
    WAITMS(2000 * (tryCount + 1));

    if (tryCount++ > 5) {
      IOTC_LOG(F("ERROR: %s endpoint %s call has failed."), dpsEndpoint,
               hostName == NULL ? "PUT" : "GET");
      return 1;
    } else {
      iotc_socket_close();
    }
  }

  StringBuffer tmpBuffer(STRING_BUFFER_1024);
  StringBuffer deviceIdEncoded(deviceId, strlen(deviceId));
  deviceIdEncoded.urlEncode();

  size_t size = 0;
  if (hostName == NULL) {
    size = strlen("{\"registrationId\":\"%s\"}") + strlen(deviceId) - 2;
    size += internal->modelData == NULL
                ? 0
                : strlen(internal->modelData) + 8;  // ,\"data\":
    const char* preModalData = internal->modelData == NULL ? "" : ",\"data\":";
    const char* apiDate =
        internal->modelData == NULL ? "2018-11-01" : "2019-01-15";
    size = snprintf(*tmpBuffer, STRING_BUFFER_1024,
                    "\
PUT /%s/registrations/%s/register?api-version=%s HTTP/1.1\r\n\
Host: %s\r\n\
content-type: application/json; charset=utf-8\r\n\
%s\r\n\
accept: */*\r\n\
content-length: %d\r\n\
%s\r\n\
connection: close\r\n\
\r\n\
{\"registrationId\":\"%s\"%s%s}\r\n\
    ",
                    scopeId, *deviceIdEncoded, apiDate, dpsEndpoint,
                    AZURE_IOT_CENTRAL_CLIENT_SIGNATURE, size, authHeader,
                    deviceId, preModalData,
                    internal->modelData == NULL ? "" : internal->modelData);
  } else {
    size = snprintf(
        *tmpBuffer, STRING_BUFFER_1024,
        F("GET /%s/registrations/%s/operations/%s?api-version=2018-11-01 HTTP/1.1\r\n\
Host: %s\r\n\
content-type: application/json; charset=utf-8\r\n\
%s\r\n\
accept: */*\r\n\
%s\r\n\
connection: close\r\n\
\r\n"),
        scopeId, *deviceIdEncoded, operationId, dpsEndpoint,
        AZURE_IOT_CENTRAL_CLIENT_SIGNATURE, authHeader);
  }
  assert(size != 0 && size < STRING_BUFFER_1024);
  tmpBuffer.setLength(size);

  const char* lookFor =
      hostName == NULL ? "{\"operationId\":\"" : "\"assignedHub\":\"";
  int index = 0;
  uint32_t rec_size = 0;

  if (iotc_socket_send(*tmpBuffer, tmpBuffer.getLength()) != pdPASS) {
    IOTC_LOG("ERROR: iotc_socket_send has failed during %s call.",
             hostName == NULL ? "PUT" : "GET");
    exitCode = 1;
    goto exit_operationId;
  }

  WAITMS(1500);
  memset(*tmpBuffer, 0, STRING_BUFFER_1024);
  if (iotc_socket_read(*tmpBuffer, STRING_BUFFER_1024, &rec_size) != pdPASS) {
    IOTC_LOG("ERROR: iotc_socket_read has failed during %s call.",
             hostName == NULL ? "PUT" : "GET");
    exitCode = 1;
    goto exit_operationId;
  }
  assert(rec_size <= STRING_BUFFER_1024);
  tmpBuffer.setLength(rec_size);

  index = tmpBuffer.indexOf(lookFor, strlen(lookFor), 0);
  if (index == -1) {
  error_exit:
    IOTC_LOG(F("ERROR: DPS (%s) request has failed.\r\n%s"),
             hostName == NULL ? "PUT" : "GET", *tmpBuffer);
    exitCode = 1;
    goto exit_operationId;
  } else {
    index += strlen(lookFor);
    int index2 = tmpBuffer.indexOf("\"", 1, index + 1);
    if (index2 == -1) goto error_exit;
    tmpBuffer.setLength(index2);
    strcpy(hostName == NULL ? operationId : hostName, (*tmpBuffer) + index);
  }

exit_operationId:
  iotc_socket_close();
  return exitCode;
}

int getHubHostName(IOTContextInternal* internal, const char* dpsEndpoint,
                   const char* scopeId, const char* deviceId, const char* key,
                   char* hostName) {
  int tryCount = 0;
  int retval = 0;
try_getHubHostName : {
  StringBuffer authHeader(STRING_BUFFER_256);
  size_t size = 0;

  IOTC_LOG(F("- iotc.dps : getting auth..."));
  if (getDPSAuthString(scopeId, deviceId, key, *authHeader, STRING_BUFFER_256,
                       size)) {
    IOTC_LOG(F("ERROR: getDPSAuthString has failed"));
    return 1;
  }

  IOTC_LOG(F("- iotc.dps : getting operation id..."));
  StringBuffer operationId(STRING_BUFFER_64);

  if ((retval = _getOperationId(internal, dpsEndpoint, scopeId, deviceId,
                                *authHeader, *operationId, NULL)) == 0) {
    WAITMS(5000);
    IOTC_LOG(F("- iotc.dps : getting host name..."));
    for (int i = 0; i < 5; i++) {
      retval = _getOperationId(internal, dpsEndpoint, scopeId, deviceId,
                               *authHeader, *operationId, hostName);
      if (retval == 0) break;
      WAITMS(4000);
    }
  } else {
    if (tryCount++ < 5) goto try_getHubHostName;
  }
}  // try_getHubHostName

  return retval;
}

/* extern */
int iotc_free_context(IOTContext ctx) {
  MUST_CALL_AFTER_INIT(ctx);

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  if (internal->endpoint != NULL) {
    IOTC_FREE(internal->endpoint);
  }

  if (internal->modelData != NULL) {
    IOTC_FREE(internal->modelData);
    internal->modelData = NULL;
  }

  if (internal->mqttClient != NULL) {
    iotc_disconnect(ctx);
  }

  IOTC_FREE(internal);

  setSingletonContext(NULL);

  return 0;
}

extern "C" void CRYPTO_ConfigureHeap(void);

static char* hostNameCache = NULL;

int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
                 const char* deviceId, IOTConnectType type) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL(keyORcert, 512);

  CRYPTO_ConfigureHeap();

  StringBuffer hostName;
  StringBuffer username;
  StringBuffer password;
  IOTContextInternal* internal = (IOTContextInternal*)ctx;

  if (type == IOTC_CONNECT_CONNECTION_STRING) {
    getUsernameAndPasswordFromConnectionString(keyORcert, strlen(keyORcert),
                                               hostName, internal->deviceId,
                                               username, password);
  } else if (type == IOTC_CONNECT_SYMM_KEY) {
    assert(scope != NULL && deviceId != NULL);
    if (hostNameCache == NULL) {
      hostNameCache = (char*) IOTC_MALLOC(STRING_BUFFER_128 + 1);
      if (getHubHostName(
              internal,
              internal->endpoint == NULL ? DEFAULT_ENDPOINT : internal->endpoint,
              scope, deviceId, keyORcert, hostNameCache)) {
        return 1;
      }
    }

    StringBuffer cstr(STRING_BUFFER_256);
    int rc = snprintf(*cstr, STRING_BUFFER_256,
                      F("HostName=%s;DeviceId=%s;SharedAccessKey=%s"),
                      hostNameCache, deviceId, keyORcert);
    assert(rc > 0 && rc < STRING_BUFFER_256);
    cstr.setLength(rc);

    // TODO: move into iotc_dps and do not re-parse from connection string
    getUsernameAndPasswordFromConnectionString(
        *cstr, rc, hostName, internal->deviceId, username, password);
  } else if (type == IOTC_CONNECT_X509_CERT) {
    IOTC_LOG(F("ERROR: IOTC_CONNECT_X509_CERT NOT IMPLEMENTED"));
    connectionStatusCallback(IOTC_CONNECTION_DEVICE_DISABLED,
                             (IOTContextInternal*)ctx);
    return 1;
  }

  if (iotc_mqtt_connect(*hostName, *username, *password) != 0) {
    IOTC_LOG(
        F("ERROR: MQTT client connect attempt failed. Check host, deviceId, "
          "username and password."));
    connectionStatusCallback(IOTC_CONNECTION_BAD_CREDENTIAL,
                             (IOTContextInternal*)ctx);
    return 1;
  }

  StringBuffer buffer(STRING_BUFFER_64);
  size_t size = snprintf(*buffer, 63, "devices/%s/messages/events/#",
                         *internal->deviceId);
  buffer.setLength(size);

  int errorCode = iotc_mqtt_subscribe(*buffer);

  size = snprintf(*buffer, 63, "devices/%s/messages/devicebound/#",
                  *internal->deviceId);
  buffer.setLength(size);

  errorCode += iotc_mqtt_subscribe(*buffer);

  errorCode += iotc_mqtt_subscribe(
      "$iothub/twin/PATCH/properties/desired/#");  // twin desired property
                                                   // changes
  errorCode +=
      iotc_mqtt_subscribe("$iothub/twin/res/#");  // twin properties response
  errorCode += iotc_mqtt_subscribe("$iothub/methods/POST/#");

  if (errorCode != 0) {
    IOTC_LOG(F("ERROR: mqttClient couldn't subscribe to hub events. "
               "error code sum => %d"),
             errorCode);
  }

  connectionStatusCallback(IOTC_CONNECTION_OK, (IOTContextInternal*)ctx);
  return 0;
}

/* extern */
int iotc_disconnect(IOTContext ctx) {
  CHECK_NOT_NULL(ctx)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  if (internal->mqttClient) {
    MQTT_AGENT_Disconnect(internal->mqttClient,
                          IOTC_SERVER_RESPONSE_TIMEOUT * 1000);
    MQTT_AGENT_Delete(internal->mqttClient);
    internal->mqttClient = NULL;
  }

  connectionStatusCallback(IOTC_CONNECTION_DISCONNECTED,
                           (IOTContextInternal*)ctx);
  return 0;
}

/* extern */
int iotc_do_work(IOTContext ctx) {
  CHECK_NOT_NULL(ctx)

  // IOTContextInternal *internal = (IOTContextInternal*)ctx;
  // MUST_CALL_AFTER_CONNECT(internal);
  sendDoNodes();

  // NO OP
  return 0;
}

/* extern */
int iotc_set_network_interface(void* networkInterface) {
  // NO OP
  return 0;
}

#endif  // defined(PLATFORM_FREERTOS)
