// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if defined(__MBED__)
#include "../common/iotc_platform.h"
#if defined(USE_LIGHT_CLIENT)

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "../common/json.h"
#include "../common/iotc_internal.h"

unsigned long getNow() {
    return AzureIOT::TLSClient::nowTime();
}

int socketReadThemAll(AzureIOT::TLSClient *client, AzureIOT::StringBuffer &readBuffer) {
    int retryCount = 0;

retry_getOperationId_read:
    char *readBufferCSTR = *readBuffer;
    WAITMS(3000); // give 3 secs to server to process

    int index = 0;
    while (index < STRING_BUFFER_1024 - 1) {
        unsigned char buff[8];
        int rc = client->read(buff, 8, 2);
        if (rc <= 0) {
            if (rc == NSAPI_ERROR_WOULD_BLOCK)
                continue;
            else
                break;
        }
        const int inc = iotc_min(rc, STRING_BUFFER_1024 - index);
        memcpy(readBufferCSTR, buff, inc);
        index += inc;
        readBufferCSTR += rc;
    }
    assert(index < STRING_BUFFER_1024 - 1);
    if (index >= 0) {
        readBuffer.setLength(index);

        if (index == 0) {
            if (retryCount > 2) return 1;
            retryCount++;
            goto retry_getOperationId_read;
        }
    } else {
        readBuffer.setLength(0);
        IOTC_LOG(F("ERROR: (socketReadThemAll) client->read returned %d"), index);
        return 1;
    }

    return 0;
}

int _getOperationId(const char* dpsEndpoint, const char* scopeId, const char* deviceId,
    const char* authHeader, char *operationId, char *hostName) {
    AzureIOT::TLSClient *client = new AzureIOT::TLSClient();
    int exitCode = 0;

    if (client->connect(dpsEndpoint, AZURE_HTTPS_SERVER_PORT) != 0) {
        IOTC_LOG(F("ERROR: %s endpoint PUT call has failed."), hostName == NULL ? "PUT" : "GET");
        delete client;
        return 1;
    }

    AzureIOT::StringBuffer tmpBuffer(STRING_BUFFER_1024);
    AzureIOT::StringBuffer deviceIdEncoded(deviceId, strlen(deviceId));
    deviceIdEncoded.urlEncode();

    size_t size = 0;
    if (hostName == NULL) {
        size = sprintf(NULL, "{\"registrationId\":\"%s\"}", deviceId);
        size = snprintf(*tmpBuffer, STRING_BUFFER_1024, F("\
PUT /%s/registrations/%s/register?api-version=2018-11-01 HTTP/1.1\r\n\
Host: %s\r\n\
content-type: application/json; charset=utf-8\r\n\
%s\r\n\
accept: */*\r\n\
content-length: %d\r\n\
%s\r\n\
connection: close\r\n\
\r\n\
{\"registrationId\":\"%s\"}\r\n\
    "),
        scopeId, *deviceIdEncoded, dpsEndpoint,
        AZURE_IOT_CENTRAL_CLIENT_SIGNATURE, size, authHeader, deviceId);
    } else {
        size = snprintf(*tmpBuffer, STRING_BUFFER_1024, F("\
GET /%s/registrations/%s/operations/%s?api-version=2018-11-01 HTTP/1.1\r\n\
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

    const char* lookFor=  hostName == NULL ? "{\"operationId\":\"" : "\"assignedHub\":\"";
    int index = 0;

    size_t total = 0;
    while (size > total) {
        int sent = client->write((const unsigned char*)*tmpBuffer + total, strlen(*tmpBuffer + total), 100);
        if (sent < 0) {
            IOTC_LOG(F("ERROR: tlsSocket send has failed with error code %d. Remaining buffer size was %lu"), sent, size);
            exitCode = 1;
            goto exit_operationId;
        }
        total += sent;
    }

    WAITMS(2000);
    memset(*tmpBuffer, 0, STRING_BUFFER_1024);
    if (socketReadThemAll(client, tmpBuffer) != 0) goto error_exit;

    index = tmpBuffer.indexOf(lookFor, strlen(lookFor), 0);
    if (index == -1) {
error_exit:
        IOTC_LOG(F("ERROR: DPS (%s) request has failed.\r\n%s"), hostName == NULL ? "PUT" : "GET", *tmpBuffer);
        exitCode = 1;
        goto exit_operationId;
    } else {
        index += strlen(lookFor);
        int index2 = tmpBuffer.indexOf("\"", 1, index + 1);
        if (index2 == -1) goto error_exit;
        tmpBuffer.setLength(index2);
        strcpy(hostName == NULL ? operationId : hostName, (*tmpBuffer) + index);
        client->disconnect();
    }

exit_operationId:
    delete client;
    return exitCode;
}

int getHubHostName(const char* dpsEndpoint, const char *scopeId, const char* deviceId, const char* key, char *hostName) {
    AzureIOT::StringBuffer authHeader(STRING_BUFFER_256);
    size_t size = 0;

    IOTC_LOG(F("- iotc.dps : getting auth..."));
    if (getDPSAuthString(scopeId, deviceId, key, *authHeader, STRING_BUFFER_256, size)) {
        IOTC_LOG(F("ERROR: getDPSAuthString has failed"));
        return 1;
    }
    IOTC_LOG(F("- iotc.dps : getting operation id..."));
    AzureIOT::StringBuffer operationId(STRING_BUFFER_64);
    int retval = 0;
    if ((retval = _getOperationId(dpsEndpoint, scopeId, deviceId, *authHeader, *operationId, NULL)) == 0) {
        WAITMS(4000);
        IOTC_LOG(F("- iotc.dps : getting host name..."));
        for (int i = 0; i < 5; i++) {
            retval = _getOperationId(dpsEndpoint, scopeId, deviceId, *authHeader, *operationId, hostName);
            if (retval == 0) break;
            WAITMS(3000);
        }
    }

    return retval;
}

static void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    unsigned long lenTopic = 0;
    char *dataTopic = NULL;
    if (md.topicName.cstring) {
        lenTopic = strlen(md.topicName.cstring);
        dataTopic = md.topicName.cstring;
    } else if (md.topicName.lenstring.len) {
        lenTopic = md.topicName.lenstring.len;
        dataTopic = md.topicName.lenstring.data;
    }

    handlePayload((char*)message.payload, message.payloadlen, dataTopic, lenTopic);
}

/* extern */
int iotc_free_context(IOTContext ctx) {
    MUST_CALL_AFTER_INIT(ctx);

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    if (internal->endpoint != NULL) {
        free(internal->endpoint);
    }

    if (internal->tlsClient != NULL) {
        iotc_disconnect(ctx);
    }

    free(internal);

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
    IOTContextInternal *internal = (IOTContextInternal*)ctx;

    if (type == IOTC_CONNECT_CONNECTION_STRING) {
        getUsernameAndPasswordFromConnectionString(keyORcert,
            strlen(keyORcert), hostName, internal->deviceId, username, password);
    } else if (type == IOTC_CONNECT_SYMM_KEY) {
        assert(scope != NULL && deviceId != NULL);
        AzureIOT::StringBuffer tmpHostname(STRING_BUFFER_128);
        if (getHubHostName(internal->endpoint == NULL ?
                DEFAULT_ENDPOINT : internal->endpoint, scope, deviceId, keyORcert, *tmpHostname)) {
            return 1;
        }
        AzureIOT::StringBuffer cstr(STRING_BUFFER_256);
        int rc = snprintf(*cstr, STRING_BUFFER_256,
            F("HostName=%s;DeviceId=%s;SharedAccessKey=%s"), *tmpHostname, deviceId, keyORcert);
        assert(rc > 0 && rc < STRING_BUFFER_256);
        cstr.setLength(rc);

        // TODO: move into iotc_dps and do not re-parse from connection string
        getUsernameAndPasswordFromConnectionString(*cstr, rc, hostName, internal->deviceId, username, password);
    } else if (type == IOTC_CONNECT_X509_CERT) {
        IOTC_LOG(F("ERROR: IOTC_CONNECT_X509_CERT NOT IMPLEMENTED"));
        connectionStatusCallback(IOTC_CONNECTION_DEVICE_DISABLED, (IOTContextInternal*)ctx);
        return 1;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.clientID.cstring = *internal->deviceId;
    data.username.cstring = *username;
    data.password.cstring = *password;
    data.keepAliveInterval = 2;
    data.cleansession = 1;

    internal->tlsClient = new AzureIOT::TLSClient();
    internal->mqttClient = new MQTT::Client<AzureIOT::TLSClient, Countdown, STRING_BUFFER_1024, 5>(*(internal->tlsClient));

    if (internal->tlsClient->connect(*hostName, AZURE_MQTT_SERVER_PORT) != 0) {
        IOTC_LOG(F("ERROR: TLSClient connect attempt failed to %s\r\nMake sure both certificate and host are correct."), *hostName);
        connectionStatusCallback(IOTC_CONNECTION_BAD_CREDENTIAL, (IOTContextInternal*)ctx);
        return 1;
    }

    if (internal->mqttClient->connect(data) != MQTT::SUCCESS) {
        IOTC_LOG(F("ERROR: TLSClient connect attempt failed. Check host, deviceId, username and password."));
        connectionStatusCallback(IOTC_CONNECTION_BAD_CREDENTIAL, (IOTContextInternal*)ctx);
        return 1;
    }

    AzureIOT::StringBuffer buffer(STRING_BUFFER_64);
    size_t size = snprintf(*buffer, 63, "devices/%s/messages/events/#", *internal->deviceId);
    buffer.setLength(size);

    int errorCode = 0;
    if ( (errorCode = internal->mqttClient->subscribe(*buffer, MQTT::QOS1, messageArrived)) != 0)
        IOTC_LOG(F("ERROR: mqttClient couldn't subscribe to %s. error code => %d"), buffer, errorCode);

    size = snprintf(*buffer, 63, "devices/%s/messages/devicebound/#", *internal->deviceId);
    buffer.setLength(size);

    if ( (errorCode = internal->mqttClient->subscribe(*buffer, MQTT::QOS1, messageArrived)) != 0)
        IOTC_LOG(F("ERROR: mqttClient couldn't subscribe to %s. error code => %d"), buffer, errorCode);

    errorCode  = internal->mqttClient->subscribe("$iothub/twin/PATCH/properties/desired/#", MQTT::QOS1, messageArrived); // twin desired property changes
    errorCode += internal->mqttClient->subscribe("$iothub/twin/res/#", MQTT::QOS1, messageArrived); // twin properties response
    errorCode += internal->mqttClient->subscribe("$iothub/methods/POST/#", MQTT::QOS1, messageArrived);

    if (errorCode != 0)
        IOTC_LOG(F("ERROR: mqttClient couldn't subscribe to twin/methods etc. error code sum => %d"), errorCode);

    internal->mqttClient->setDefaultMessageHandler(messageArrived);
    connectionStatusCallback(IOTC_CONNECTION_OK, (IOTContextInternal*)ctx);
    return 0;
}

/* extern */
int iotc_disconnect(IOTContext ctx) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    if (internal->mqttClient) {
        if(internal->mqttClient->isConnected()) {
            internal->mqttClient->disconnect();
        }
        delete internal->mqttClient;
        internal->mqttClient = NULL;
    }

    if(internal->tlsClient) {
        internal->tlsClient->disconnect();
        delete internal->tlsClient;
        internal->tlsClient = NULL;
    }
    connectionStatusCallback(IOTC_CONNECTION_DISCONNECTED, (IOTContextInternal*)ctx);
    return 0;
}

/* extern */
int iotc_do_work(IOTContext ctx) {
    CHECK_NOT_NULL(ctx)

    IOTContextInternal *internal = (IOTContextInternal*)ctx;
    MUST_CALL_AFTER_CONNECT(internal);

    return internal->mqttClient->yield();
}

/* extern */
int iotc_set_network_interface(void* networkInterface) {
    AzureIOT::TLSClient::setNetworkInterface((NetworkInterface*)networkInterface);
    return 0;
}

#endif // defined(USE_LIGHT_CLIENT)
#endif // __MBED__