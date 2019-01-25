// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license.

#ifndef AZURE_IOTC_MBED_TLS_CLIENT_H
#define AZURE_IOTC_MBED_TLS_CLIENT_H

#if defined(__MBED__)
#include "NetworkInterface.h"
#include "TLSSocket.h"
#include "NTPClient.h"
#include <assert.h>
#include "iotc_definitions.h"

namespace AzureIOT {

class TLSClient {
    static NetworkInterface* networkInterface;
    TLSSocket* tlsSocket;

    static time_t timestamp;
    static time_t timeStart;
public:
    TLSClient() {
        tlsSocket = new TLSSocket();
        assert(tlsSocket);
    }

    int read(unsigned char* buffer, int len, int timeout);
    int write(const unsigned char* buffer, int len, int timeout);

    int print(const char* buffer);
    int println() { print("\r\n"); }
    int println(const char* buffer) { print(buffer); println(); }

    bool connect(const char* host, int port);
    bool disconnect();

    ~TLSClient() {
      delete tlsSocket;
    }

    static void setNetworkInterface(NetworkInterface* interface) {
        networkInterface = interface;
    }

    static NetworkInterface* getNetworkInterface() {
        return networkInterface;
    }

    static time_t nowTime() {
        assert(TLSClient::networkInterface != NULL);

        time_t timeDiff;

        if (timestamp == 0) {
sync_ntp:
            NTPClient ntp(TLSClient::networkInterface);
            int retry_count = 0;

            retry_time:
            {
                timeStart = us_ticker_read(); // terrible hack
                timestamp = ntp.get_timestamp(15000 /* timeout */);
                if (timestamp < 0) // timeout ?
                {
                    if (retry_count++ < 5) {
                        goto retry_time;
                    }

                    printf("- ERROR: can't sync to NTP server\r\n");
                    return 0;
                }
            }
        }

        timeDiff = us_ticker_read() - timeStart;
        time_t time_now = timestamp + (10 /* lag */ + (timeDiff / 1000));

        if (time_now - timestamp > NTP_SYNC_PERIOD) {
            goto sync_ntp;
        }
        return time_now;
    }
};

} // namespace AzureIOT

#endif // __MBED__

#endif // AZURE_IOTC_MBED_TLS_CLIENT_H
