// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "../common/iotc_internal.h"
#include "../common/iotc_json.h"
#include "../common/iotc_platform.h"

#include "aws_secure_sockets.h"

void iotc_socket_close() { SOCKETS_Close(getSingletonContext()->xSocket); }

int iotc_socket_open() {
  assert(getSingletonContext() != NULL);
  const char *endpoint = getSingletonContext()->endpoint == NULL
                             ? DEFAULT_ENDPOINT
                             : getSingletonContext()->endpoint;

  SocketsSockaddr_t socko = {0};
  socko.usPort = SOCKETS_htons(AZURE_HTTPS_SERVER_PORT);
  socko.ulAddress = SOCKETS_GetHostByName(endpoint);
  if (socko.ulAddress == 0) {
    // possibly modem is not ready
    WAITMS(1200);
    return -1;
  }

  Socket_t xSocket =
      SOCKETS_Socket(SOCKETS_AF_INET, SOCKETS_SOCK_STREAM, SOCKETS_IPPROTO_TCP);
  getSingletonContext()->xSocket = xSocket;
  const TickType_t xReceiveTimeOut =
      pdMS_TO_TICKS(IOTC_SERVER_RESPONSE_TIMEOUT * 1000);
  const TickType_t xSendTimeOut =
      pdMS_TO_TICKS(IOTC_SERVER_RESPONSE_TIMEOUT * 1000);

  SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_SNDTIMEO, &xSendTimeOut,
                     sizeof(xSendTimeOut));
  SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_RCVTIMEO, &xReceiveTimeOut,
                     sizeof(xReceiveTimeOut));
  SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_REQUIRE_TLS, NULL, (size_t)0);
  size_t lenmemo = strlen(SSL_CA_PEM_DEF) + 1 /*\0*/;
  if (SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE,
                         SSL_CA_PEM_DEF, lenmemo) != SOCKETS_ERROR_NONE) {
    IOTC_LOG("ERROR: Couldn't set SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE");
    SOCKETS_Close(xSocket);
    return 1;
  }

  if (SOCKETS_SetSockOpt(xSocket, 0, SOCKETS_SO_SERVER_NAME_INDICATION,
                         endpoint,
                         (size_t)1 + strlen(endpoint)) != SOCKETS_ERROR_NONE) {
    IOTC_LOG("ERROR: Couldn't set SOCKETS_SO_SERVER_NAME_INDICATION");
    SOCKETS_Close(xSocket);
    return 1;
  }

  int32_t res = SOCKETS_Connect(xSocket, &socko, sizeof(socko));
  if (res != SOCKETS_ERROR_NONE) {
    SOCKETS_Close(xSocket);
    if (res != SOCKETS_TLS_HANDSHAKE_ERROR) {
      IOTC_LOG("ERROR: SOCKETS_Connect failed with error code => %d", res);
      return res;
    }
    return -1;
  }
  IOTC_LOG("- OpenSocket @global.azure-devices-provisioning.net");
  return 0;
}

long iotc_socket_send(const char *pcPayload, const uint32_t ulPayloadSize) {
  BaseType_t xStatus = pdFAIL;
  uint32_t ulBytesSent = 0;
  int32_t lSendRetVal;

  assert(pcPayload != NULL);

  if (ulPayloadSize > 0) {
    while (ulBytesSent < ulPayloadSize) {
      lSendRetVal =
          SOCKETS_Send(getSingletonContext()->xSocket,
                       (const unsigned char *)&pcPayload[ulBytesSent],
                       (size_t)(ulPayloadSize - ulBytesSent), (uint32_t)0);
      if (lSendRetVal < 0) {
        break;
      } else {
        ulBytesSent += (uint32_t)lSendRetVal;
      }
    }

    if (ulBytesSent == ulPayloadSize) {
      xStatus = pdPASS;
    } else {
      IOTC_LOG("SecureConnect - error sending secure data to network");
      xStatus = pdFAIL;
    }
  } else {
    xStatus = pdPASS;
  }

  return xStatus;
}

long iotc_socket_read(char *pcBuffer, const uint32_t ulBufferSize,
                      uint32_t *pulDataRecvSize) {
  int32_t lTmpStatus = 0;
  BaseType_t xStatus = pdFAIL;
  int retry = 0;

  assert(pulDataRecvSize != NULL);
  assert(pcBuffer != NULL);

  for (; retry < 3; retry++) {
    lTmpStatus =
        SOCKETS_Recv(getSingletonContext()->xSocket, (unsigned char *)pcBuffer,
                     (uint32_t)ulBufferSize, (uint32_t)0);

    if (lTmpStatus != 0) {
      if (lTmpStatus < 0) {
        IOTC_LOG("SecureConnect - recv error, %d", lTmpStatus);
        xStatus = pdFAIL;
      } else {
        xStatus = pdPASS;
      }

      break;
    } else {
      /* It is a timeout, retry. */
      IOTC_LOG("SecureConnect - recv Timeout");
    }
  }

  if (retry == 3) {
    IOTC_LOG("SecureConnect - recv number of Timeout exceeded");
    xStatus = pdFAIL;
  }

  *pulDataRecvSize = (uint32_t)lTmpStatus;
  return xStatus;
}

int mqtt_publish(IOTContextInternal *internal, const char *topic,
                 unsigned long topic_length, const char *msg,
                 unsigned long msg_length) {
  assert(internal && internal->mqttClient != NULL);

  MQTTAgentPublishParams_t pubpar;
  pubpar.pucTopic = (const uint8_t *)topic;
  pubpar.usTopicLength = topic_length;
  pubpar.pvData = msg;
  pubpar.ulDataLength = msg_length;
  pubpar.xQoS = eMQTTQoS1;

  MQTTAgentReturnCode_t xReturned =
      MQTT_AGENT_Publish(internal->mqttClient, &pubpar,
                         pdMS_TO_TICKS(IOTC_SERVER_RESPONSE_TIMEOUT * 1000));

  return (xReturned == eMQTTAgentSuccess) ? 0 : 1;
}

static BaseType_t mqttCallback(void *ctx,
                               const MQTTAgentCallbackParams *cparams) {
  bool isDisconnect = cparams->xMQTTEvent == eMQTTAgentDisconnect;
  const MQTTPublishData_t *pxCallbackParams = &(cparams->u.xPublishData);

  if (pxCallbackParams->usTopicLength == 0) {
    IOTC_LOG("ERROR: mqttCallback without a topic.");
    return 0;
  }

  if (isDisconnect) {
    connectionStatusCallback(IOTC_CONNECTION_DISCONNECTED,
                             (IOTContextInternal *)ctx);
    return 0;
  }

  StringBuffer topic((const char *)pxCallbackParams->pucTopic,
                     pxCallbackParams->usTopicLength);
  handlePayload((char *)pxCallbackParams->pvData,
                pxCallbackParams->ulDataLength, *topic, topic.getLength());
  return 0;
}

MQTTBool_t mqttSubCallback(void *ctx,
                           const MQTTPublishData_t *const pxPublishData) {
  if (pxPublishData->usTopicLength == 0) {
    IOTC_LOG("ERROR: mqttSubCallback without a topic.");
    return eMQTTFalse;
  }

  StringBuffer topic((const char *)pxPublishData->pucTopic,
                     pxPublishData->usTopicLength);
  handlePayload((char *)pxPublishData->pvData, pxPublishData->ulDataLength,
                *topic, topic.getLength());
  return eMQTTFalse;
}

int iotc_mqtt_subscribe(const char *topic) {
  assert(getSingletonContext() != NULL);
  IOTContextInternal *internal = getSingletonContext();
  assert(internal->mqttClient != NULL);

  MQTTAgentSubscribeParams_t subpar;
  StringBuffer buff(topic, strlen(topic));

  subpar.pucTopic = (const uint8_t *)*buff;
  subpar.pvPublishCallbackContext = internal;
  subpar.pxPublishCallback = mqttSubCallback;
  subpar.usTopicLength = (uint16_t)buff.getLength();
  subpar.xQoS = eMQTTQoS1;

  buff.data = NULL;  // prevent GC

  /* Subscribe to the topic. */
  MQTTAgentReturnCode_t xReturned =
      MQTT_AGENT_Subscribe(internal->mqttClient, &subpar,
                           pdMS_TO_TICKS(IOTC_SERVER_RESPONSE_TIMEOUT * 1000));
  return (xReturned == eMQTTAgentSuccess) ? 0 : 1;
}

int iotc_mqtt_connect(const char *hubEndpoint, char *username, char *password) {
  assert(getSingletonContext() != NULL);
  IOTContextInternal *internal = getSingletonContext();

  MQTTAgentConnectParams_t xConnectParameters = {
      hubEndpoint,            /* The URL of the MQTT broker to connect to. */
      0x00000002,             /* Connection flags => REQUIRE_TLS */
      pdFALSE,                /* Deprecated. */
      AZURE_MQTT_SERVER_PORT, /* Port number on which the MQTT broker is
                                 listening. */
      (const uint8_t *)*internal
          ->deviceId, /* Client Identifier of the MQTT client. It should be
                         unique per broker. */
      0,        /* The length of the client Id, filled in later as not const. */
      pdFALSE,  /* Deprecated. */
      internal, /* User data supplied to the callback. Can be NULL. */
      mqttCallback,   /* Callback used to report various events. Can be NULL. */
      SSL_CA_PEM_DEF, /* Certificate used for secure connection. Can be NULL. */
      0,              /* Size of certificate used for secure connection. */
      username,
      password};

  int retryCount = 0;
retry_mqtt_connect:
  assert(internal->mqttClient == NULL);
  if (MQTT_AGENT_Create(&internal->mqttClient) == eMQTTAgentSuccess) {
    xConnectParameters.usClientIdLength = internal->deviceId.getLength();
    xConnectParameters.ulCertificateSize = strlen(SSL_CA_PEM_DEF) + 1 /* \0 */;

    IOTC_LOG("MQTT connecting to %s.", hubEndpoint);
    MQTTAgentReturnCode_t xReturned =
        MQTT_AGENT_Connect(internal->mqttClient, &xConnectParameters,
                           pdMS_TO_TICKS(IOTC_SERVER_RESPONSE_TIMEOUT * 1000));

    if (xReturned != eMQTTAgentSuccess) {
      MQTT_AGENT_Delete(internal->mqttClient);
      internal->mqttClient = NULL;
      IOTC_LOG("ERROR: MQTT connect has failed. ret code: %d", xReturned);
      if (retryCount++ < 3) {
        IOTC_LOG("Retrying MQTT connect..");
        goto retry_mqtt_connect;
      }
      return 1;
    } else {
      IOTC_LOG("MQTT Connected!");
    }
    return 0;
  }

  return 1;
}
