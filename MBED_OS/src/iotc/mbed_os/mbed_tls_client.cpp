// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license.

#if defined(__MBED__)
#include "../common/iotc_internal.h"
#if defined(USE_LIGHT_CLIENT)

namespace AzureIOT {

time_t TLSClient::timestamp = 0;
time_t TLSClient::timeStart = 0;
NetworkInterface* TLSClient::networkInterface = NULL;

int TLSClient::read(unsigned char* buffer, int len, int timeout) {
    tlsSocket->set_timeout(iotc_max(timeout, 10));
    return tlsSocket->recv(buffer, len);
}

int TLSClient::write(const unsigned char* buffer, int len, int timeout) {
    tlsSocket->set_timeout(iotc_max(timeout, 10));
    return tlsSocket->send(buffer, len);
}

bool TLSClient::connect(const char* host, int port) {
    IOTC_LOG(F("TLSClient::connect host(%s)"), host);

    assert(getNetworkInterface() != NULL);
    if (tlsSocket->open(getNetworkInterface()) != 0) {
        IOTC_LOG(F("ERROR: TLSClient::connect failed"));
        return false;
    }

    tlsSocket->set_root_ca_cert((const char*)SSL_CA_PEM_DEF);
    tlsSocket->set_client_cert_key(SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
    tlsSocket->set_blocking(true);

    return tlsSocket->connect(host, port);
}

bool TLSClient::disconnect() {
    IOTC_LOG(F("TLSClient::disconnect"));
    return tlsSocket->close() == 0;
}

} // namespace AzureIOT

#endif // defined(USE_LIGHT_CLIENT)
#endif // __MBED__