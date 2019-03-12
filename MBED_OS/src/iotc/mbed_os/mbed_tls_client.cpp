// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license.

#if defined(__MBED__)
#include "../common/iotc_internal.h"
#if defined(USE_LIGHT_CLIENT)

namespace AzureIOT
{

time_t TLSClient::timestamp = 0;
time_t TLSClient::timeStart = 0;
NetworkInterface *TLSClient::networkInterface = NULL;
bool open = false;


bool TLSClient::connect(const char *host, int port)
{
    IOTC_LOG(F("TLSClient::connect host(%s) at port %d"), host, port);

    assert(getNetworkInterface() != NULL);
    if (tlsSocket->open(getNetworkInterface()) != 0)
    {
        IOTC_LOG(F("ERROR: TLSClient::connect failed"));
        return false;
    }
    open = true;
    tlsSocket->set_root_ca_cert(SSL_CA_PEM_DEF);
    tlsSocket->set_client_cert_key(SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
    tlsSocket->set_blocking(true);
    bool rc = tlsSocket->connect(host, port);
    tlsSocket->set_blocking(false); // blocking timeouts seem not to work
    return rc;
}

bool TLSClient::disconnect()
{
    IOTC_LOG(F("TLSClient::disconnect"));
    open = false;
    return tlsSocket->close() == 0;
}

int TLSClient::common(unsigned char *buffer, int len, int timeout, bool read)
{
    timer.start();
    tlsSocket->set_blocking(false); // blocking timeouts seem not to work
    int bytes = 0;
    bool first = true;
    do
    {
        if (first)
            first = false;
        else
            wait_ms(timeout < 100 ? timeout : 100);
        int rc;
        if (read)
            rc = tlsSocket->recv((char *)buffer, len);
        else
            rc = tlsSocket->send((char *)buffer, len);
        if (rc < 0)
        {
            if (rc != NSAPI_ERROR_WOULD_BLOCK)
            {
                bytes = -1;
                break;
            }
        }
        else
            bytes += rc;
    } while (bytes < len && timer.read_ms() < timeout);
    timer.stop();
    return bytes;
}

/* returns the number of bytes read, which could be 0.
       -1 if there was an error on the socket
    */
int TLSClient::read(unsigned char *buffer, int len, int timeout)
{
    timeout = iotc_max(timeout, 10);
    return TLSClient::common(buffer, len, timeout, true);
} // namespace AzureIOT

int TLSClient::write(unsigned char *buffer, int len, int timeout)
{
    timeout = iotc_max(timeout, 10);
    return TLSClient::common(buffer, len, timeout, false);
}

} // namespace AzureIOT

#endif // defined(USE_LIGHT_CLIENT)
#endif // __MBED__