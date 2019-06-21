// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#include "../common/iotc_platform.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/iotc_internal.h"
#include "../common/iotc_json.h"

unsigned long _getNow() {
  int retryCount = 0;
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[NTP_PACKET_SIZE];
  unsigned long retVal = 0;

retry_getNow:
  IPAddress address(129, 6, 15,
                    retryCount % 2 == 1 ? 28 : 29);  // time.nist.gov NTP server
  WiFiUDP Udp;
  if (Udp.begin(2390) == 0) {
    if (retryCount < 5) {
      retryCount++;
      goto retry_getNow;
    }
    IOTC_LOG(F("ERROR: couldn't fetch the time from NTP."));
    return retVal;
  }

  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();

  // wait to see if a reply is available
  WAITMS(1000);

  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    retVal = (secsSince1900 - 2208988800UL);
  } else {
    if (retryCount < 5) {
      retryCount++;
      goto retry_getNow;
    }
    IOTC_LOG(F("ERROR: couldn't fetch the time from NTP."));
  }

  Udp.stop();
  return retVal;
}

static unsigned long g_udpTime = 0, g_lastRead = 0;
unsigned long getNow() {
  unsigned long ms = millis();
  if (g_udpTime == 0 || ms - g_lastRead > NTP_SYNC_PERIOD) {
    g_udpTime = _getNow();
    g_lastRead = ms;
  }
  return g_udpTime + ((ms - g_lastRead) / 1000);
}

int _getOperationId(IOTContextInternal* internal, const char* dpsEndpoint,
                    const char* scopeId, const char* deviceId,
                    const char* authHeader, char* operationId, char* hostName) {
  ARDUINO_WIFI_SSL_CLIENT client;
  int exitCode = 0;

#ifndef USES_WIFI101
#ifndef AXTLS_DEPRECATED
  client.setCACert((const uint8_t*)SSL_CA_PEM_DEF,
                   strlen((const char*)SSL_CA_PEM_DEF));
#else   // AXTLS_DEPRECATED
  BearSSL::X509List certList(SSL_CA_PEM_DEF);
  client.setX509Time(g_udpTime);
  client.setTrustAnchors(&certList);
#endif  // AXTLS_DEPRECATED
#endif  // USES_WIFI101

  int retry = 0;
  while (retry < 5 && !client.connect(dpsEndpoint, AZURE_HTTPS_SERVER_PORT))
    retry++;
  if (!client.connected()) {
    IOTC_LOG(F("ERROR: DPS endpoint %s call has failed."),
             hostName == NULL ? "PUT" : "GET");
    return 1;
  }

  AzureIOT::StringBuffer tmpBuffer(STRING_BUFFER_1024);
  AzureIOT::StringBuffer deviceIdEncoded(deviceId, strlen(deviceId));
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
    size = snprintf(*tmpBuffer, STRING_BUFFER_1024,
                    "\
GET /%s/registrations/%s/operations/%s?api-version=2018-11-01 HTTP/1.1\r\n\
Host: %s\r\n\
content-type: application/json; charset=utf-8\r\n\
%s\r\n\
accept: */*\r\n\
%s\r\n\
connection: close\r\n\
\r\n",
                    scopeId, *deviceIdEncoded, operationId, dpsEndpoint,
                    AZURE_IOT_CENTRAL_CLIENT_SIGNATURE, authHeader);
  }
  assert(size != 0 && size < STRING_BUFFER_1024);
  tmpBuffer.setLength(size);

  client.println(*tmpBuffer);
  int index = 0;
  while (!client.available()) {
    WAITMS(100);
    if (index++ > IOTC_SERVER_RESPONSE_TIMEOUT * 10) {
      // 20 secs..
      client.stop();
      IOTC_LOG(F("ERROR: DPS (%s) request has failed. (Server didn't answer "
                 "within 20 secs.)"),
               hostName == NULL ? "PUT" : "GET");
      return 1;
    }
  }

  index = 0;
  bool enableSaving = false;
  while (client.available() && index < STRING_BUFFER_1024 - 1) {
    char ch = (char)client.read();
    if (ch == '{') {
      enableSaving = true;  // don't use memory for headers
    }

    if (enableSaving) {
      (*tmpBuffer)[index++] = ch;
    }
  }
  tmpBuffer.setLength(index);

  const char* lookFor =
      hostName == NULL ? "{\"operationId\":\"" : "\"assignedHub\":\"";
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
  client.stop();
  return exitCode;
}

int getHubHostName(IOTContextInternal* internal, const char* dpsEndpoint,
                   const char* scopeId, const char* deviceId, const char* key,
                   char* hostName) {
  AzureIOT::StringBuffer authHeader(STRING_BUFFER_256);
  size_t size = 0;

  IOTC_LOG(F("- iotc.dps : getting auth..."));
  if (getDPSAuthString(scopeId, deviceId, key, *authHeader, STRING_BUFFER_256,
                       size)) {
    IOTC_LOG(F("ERROR: getDPSAuthString has failed"));
    return 1;
  }
  IOTC_LOG(F("- iotc.dps : getting operation id..."));
  AzureIOT::StringBuffer operationId(STRING_BUFFER_64);
  int retval = 0;
  if ((retval = _getOperationId(internal, dpsEndpoint, scopeId, deviceId,
                                *authHeader, *operationId, NULL)) == 0) {
    WAITMS(2500);
    IOTC_LOG(F("- iotc.dps : getting host name..."));
    WAITMS(2500);
    for (int i = 0; i < 5; i++) {
      retval = _getOperationId(internal, dpsEndpoint, scopeId, deviceId,
                               *authHeader, *operationId, hostName);
      if (retval == 0) break;
      WAITMS(3000);
      IOTC_LOG(F("- iotc.dps : getting host name...")); // re-trigger WD
    }
  }

  return retval;
}

static void messageArrived(char* topic, byte* data, unsigned int length) {
  handlePayload((char*)data, length, topic, topic ? strlen(topic) : 0);
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

  iotc_disconnect(ctx);

  IOTC_FREE(internal);
  setSingletonContext(NULL);

  return 0;
}

int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
                 const char* deviceId, IOTConnectType type) {
  CHECK_NOT_NULL(ctx)
  GET_LENGTH_NOT_NULL(keyORcert, 512);

  AzureIOT::StringBuffer hostName;
  AzureIOT::StringBuffer username;
  AzureIOT::StringBuffer password;
  IOTContextInternal* internal = (IOTContextInternal*)ctx;

  if (type == IOTC_CONNECT_CONNECTION_STRING) {
    getUsernameAndPasswordFromConnectionString(keyORcert, strlen(keyORcert),
                                               hostName, internal->deviceId,
                                               username, password);
  } else if (type == IOTC_CONNECT_SYMM_KEY) {
    assert(scope != NULL && deviceId != NULL);
    AzureIOT::StringBuffer tmpHostname(STRING_BUFFER_128);
    if (getHubHostName(
            internal,
            internal->endpoint == NULL ? DEFAULT_ENDPOINT : internal->endpoint,
            scope, deviceId, keyORcert, *tmpHostname)) {
      return 1;
    }
    AzureIOT::StringBuffer cstr(STRING_BUFFER_256);
    int rc = snprintf(*cstr, STRING_BUFFER_256,
                      "HostName=%s;DeviceId=%s;SharedAccessKey=%s",
                      *tmpHostname, deviceId, keyORcert);
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

  internal->tlsClient = new ARDUINO_WIFI_SSL_CLIENT();

#ifndef USES_WIFI101
#ifndef AXTLS_DEPRECATED
  internal->tlsClient->setCACert((const uint8_t*)SSL_CA_PEM_DEF,
                                 strlen((const char*)SSL_CA_PEM_DEF));
#else   // AXTLS_DEPRECATED
  BearSSL::X509List certList(SSL_CA_PEM_DEF);
  internal->tlsClient->setX509Time(g_udpTime);
  internal->tlsClient->setTrustAnchors(&certList);
#endif  // AXTLS_DEPRECATED
#endif  // USES_WIFI101

  internal->mqttClient =
      new PubSubClient(*hostName, AZURE_MQTT_SERVER_PORT, internal->tlsClient);
  internal->mqttClient->setCallback(messageArrived);

  int retry = 0;
  while (retry < 10 && !internal->mqttClient->connected()) {
    if (internal->mqttClient->connect(*internal->deviceId, *username,
                                      *password)) {
      break;
    } else {
      WAITMS(2000);
      retry++;
    }
  }

  if (!internal->mqttClient->connected()) {
    IOTC_LOG(F("ERROR: MQTT client connect attempt failed. Check host, "
               "deviceId, username and password. (state %d)"),
             internal->mqttClient->state());
    connectionStatusCallback(IOTC_CONNECTION_BAD_CREDENTIAL,
                             (IOTContextInternal*)ctx);
    delete internal->tlsClient;
    delete internal->mqttClient;
    internal->tlsClient = NULL;
    internal->mqttClient = NULL;
    return 1;
  }

  const unsigned bufferLength = internal->deviceId.getLength() + STRING_BUFFER_64;
  AzureIOT::StringBuffer buffer(bufferLength);

  int errorCode = 0;
  buffer.setLength(snprintf(*buffer, bufferLength, "devices/%s/messages/events/#",
                            *internal->deviceId));
  errorCode = internal->mqttClient->subscribe(*buffer);

  buffer.setLength(snprintf(*buffer, bufferLength, "devices/%s/messages/devicebound/#",
                            *internal->deviceId));
  errorCode += internal->mqttClient->subscribe(*buffer);

  errorCode += internal->mqttClient->subscribe(
      "$iothub/twin/PATCH/properties/desired/#");  // twin desired property
                                                   // changes

  errorCode += internal->mqttClient->subscribe(
      "$iothub/twin/res/#");  // twin properties response
  errorCode += internal->mqttClient->subscribe("$iothub/methods/POST/#");

  if (errorCode < 5) {
    IOTC_LOG(F("ERROR: mqttClient couldn't subscribe to twin/methods etc. "
               "error code sum => %d"),
             errorCode);
  }

  connectionStatusCallback(IOTC_CONNECTION_OK, (IOTContextInternal*)ctx);

  iotc_do_work(internal);
  iotc_get_device_settings(ctx);  // ask for the latest device settings
  iotc_do_work(internal);

  return 0;
}

/* extern */
int iotc_disconnect(IOTContext ctx) {
  CHECK_NOT_NULL(ctx)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  if (internal->mqttClient) {
    if (internal->mqttClient->connected()) {
      internal->mqttClient->disconnect();
    }
    delete internal->mqttClient;
    internal->mqttClient = NULL;
  }

  if (internal->tlsClient) {
    delete internal->tlsClient;
    internal->tlsClient = NULL;
  }

  connectionStatusCallback(IOTC_CONNECTION_DISCONNECTED,
                           (IOTContextInternal*)ctx);
  return 0;
}

/* extern */
int iotc_do_work(IOTContext ctx) {
  CHECK_NOT_NULL(ctx)

  IOTContextInternal* internal = (IOTContextInternal*)ctx;
  MUST_CALL_AFTER_CONNECT(internal);

  if (!internal->mqttClient->loop()) {
    if (!internal->mqttClient->connected()) {
      connectionStatusCallback(IOTC_CONNECTION_DISCONNECTED,
                               (IOTContextInternal*)ctx);
    }
    return 1;
  }
  return 0;
}

/* extern */
int iotc_set_network_interface(void* networkInterface) {
  // NO-OP
  return 0;
}