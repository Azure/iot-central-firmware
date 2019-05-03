// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#define MQTT_DEBUG 1
#include <stdlib.h>

#include "mbed.h"
#define ETHERNET 1
#define WIFI_ESP8266 2
#define MESH_LOWPAN_ND 3
#define MESH_THREAD 4
#define WIFI_ODIN 5
#define WIFI_REALTEK 6
#define CELLULAR_UBLOX 7

#ifdef MBED_CONF_APP_NETWORK_INTERFACE == CELLULAR_UBLOX
#include "UbloxATCellularInterface.h"
#include "OnboardCellularInterface.h"
#define INTERFACE_CLASS UbloxATCellularInterface
#endif

#include <string.h>

#include "src/iotc/iotc.h"
#include "src/iotc/common/string_buffer.h"


static IOTContext context = NULL;
static bool isConnected = false;

static UDPSocket sockUdp;
static SocketAddress udpServer;
static SocketAddress udpSenderAddress;
static char buf[1024];
static int x;

void onEvent(IOTContext ctx, IOTCallbackInfo *callbackInfo)
{
    if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0)
    {
        LOG_VERBOSE("Is connected ? %s (%d)", callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO", callbackInfo->statusCode);
        isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    }

    AzureIOT::StringBuffer buffer;
    if (callbackInfo->payloadLength > 0)
    {
        buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
    }
    LOG_VERBOSE("- [%s] event was received. Payload => %s", callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

    if (strcmp(callbackInfo->eventName, "Command") == 0)
    {
        LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
    }
}

static unsigned prevMillis = 0, loopId = 0;
void loop()
{
    if (isConnected)
    {
        unsigned long ms = us_ticker_read() / 1000;
        if (ms - prevMillis > 15000)
        { // send telemetry every 15 seconds
            char msg[128] = {0};
            int pos = 0, errorCode = 0;

            prevMillis = ms;
            if (loopId++ % 2 == 0)
            { // send telemetry
                pos = snprintf(msg, sizeof(msg) - 1, "{\"accelerometerX\": %d,\"accelerometerY\": %d,\"accelerometerZ\": %d,\"temp\": %d}", 10 + (rand() % 20), 6 + (rand() % 20), 18 + (rand() % 20), 10 + (rand() % 20));
                errorCode = iotc_send_telemetry(context, msg, pos);
            }
            else
            { // send property
                pos = snprintf(msg, sizeof(msg) - 1, "{\"sold\":%d}", 1 + (rand() % 5));
                errorCode = iotc_send_property(context, msg, pos);
            }
            msg[pos] = 0;

            if (errorCode != 0)
            {
                LOG_ERROR("Sending message has failed with error code %d", errorCode);
            }
        }

        iotc_do_work(context); // do background work for iotc
    }
}

static void printNtpTime(NetworkInterface *interface)
{
    LOG_VERBOSE("Getting the IP address of \"2.pool.ntp.org\"...\n");
    if ((interface->gethostbyname("2.pool.ntp.org", &udpServer) == 0))
    {
        udpServer.set_port(123);
        LOG_VERBOSE("\"2.pool.ntp.org\" address: %s on port %d.\n", udpServer.get_ip_address(), udpServer.get_port())
        // UDP Sockets
        LOG_VERBOSE("=== UDP ===\n");
        LOG_VERBOSE("Opening a UDP socket...\n");
        if (sockUdp.open(interface) == 0)
        {
            LOG_VERBOSE("UDP socket open.\n");
            sockUdp.set_timeout(10000);
            LOG_VERBOSE("Sending time request to \"2.pool.ntp.org\" over UDP socket...\n");
            memset(buf, 0, sizeof(buf));
            *buf = '\x1b';
            if (sockUdp.sendto(udpServer, (void *)buf, 48) == 48)
            {
                LOG_VERBOSE("Socket send completed, waiting for UDP response...\n");
                x = sockUdp.recvfrom(&udpSenderAddress, buf, sizeof(buf));
                if (x > 0)
                {
                    LOG_VERBOSE("Received %d byte response from server %s on UDP socket:\n"
                                "-------------------------------------------------------\n",
                                x, udpSenderAddress.get_ip_address());

                    time_t timestamp = 0;
                    struct tm *localTime;
                    char timeString[25];
                    time_t TIME1970 = 2208988800U;

                    if (x >= 43)
                    {
                        timestamp |= ((int)*(buf + 40)) << 24;
                        timestamp |= ((int)*(buf + 41)) << 16;
                        timestamp |= ((int)*(buf + 42)) << 8;
                        timestamp |= ((int)*(buf + 43));
                        timestamp -= TIME1970;
                        localTime = localtime(&timestamp);
                        if (localTime)
                        {
                            if (strftime(timeString, sizeof(timeString), "%a %b %d %H:%M:%S %Y", localTime) > 0)
                            {
                                LOG_VERBOSE("NTP timestamp is %s.\n", timeString);
                            }
                        }
                    }
                    LOG_VERBOSE("-------------------------------------------------------\n");
                }
            }
            LOG_VERBOSE("Closing socket...\n");
            sockUdp.close();
            LOG_VERBOSE("Socket closed.\n");
        }
    }
}

int main(int argc, char *argv[])
{
    LOG_VERBOSE("Welcome to IoTCentral Ublox Environment");
    LOG_VERBOSE("Mbed OS version: %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    NetworkInterface *network = 0;
#if MBED_CONF_APP_NETWORK_INTERFACE == CELLULAR_UBLOX
    LOG_VERBOSE("Running on cellular interface");
    INTERFACE_CLASS *interface = new INTERFACE_CLASS();
    // INTERFACE_CLASS *network = new INTERFACE_CLASS(MDMTXD, MDMRXD,
    //                                                  115200,
    //                                                  true);
    interface->set_credentials(MBED_CONF_APP_APN, MBED_CONF_APP_USERNAME, MBED_CONF_APP_PASSWORD);
    int i;
    for (i = 0; interface->connect(MBED_CONF_APP_DEFAULT_PIN) != 0; i++)
    {
        if (i > 0)
        {
            LOG_VERBOSE("Retrying (have you checked that an antenna is plugged in and your APN is correct?)...\n");
        }
    }
    network = interface;
#else
    LOG_VERBOSE("Running on ethernet connection");
    network = NetworkInterface::get_default_instance();
    network->connect();
    if (!network)
    {
        LOG_VERBOSE("Unable to open network interface.");
        return -1;
    }
    LOG_VERBOSE("Ip address: %s", network->get_ip_address());
#endif
    printNtpTime(network);

    // NetworkInterface *network = NetworkInterface::get_default_instance();
    // if (!network)
    // {
    //     LOG_VERBOSE("Unable to open network interface.");
    //     return -1;
    // }

    // // if this sample is unable to pass NTP phase. enable the logic below to fix DNS issue
    // // it is a silly hack to set dns server as a router assuming router is sitting at .1

    // LOG_VERBOSE("Network interface opened successfully.");
    // WiFiInterface *wifi = network->wifiInterface();
    // EthInterface *eth = network->ethInterface();
    // CellularBase *cell = network->cellularBase();
    // printf("Available interfaces...");
    // if (wifi)
    // {
    //     printf("WIFI,");
    // }
    // if (eth)
    // {
    //     printf("ETHERNET,");
    // }
    // if (cell)
    // {
    //     printf("CELLULAR");
    // }
    // network->connect();
    // LOG_VERBOSE("Ip address: %s", network->get_ip_address());

    // call WiFi-specific methods

    // AzureIOT::StringBuffer routerAddress(network->get_ip_address(), strlen(network->get_ip_address()));
    // {
    //     int dotIndex = 0, foundCount = 0;
    //     for (int i = 0; i < 3; i++)
    //     {
    //         int pre = dotIndex;
    //         dotIndex = routerAddress.indexOf(".", 1, dotIndex + 1);
    //         if (dotIndex > pre)
    //         {
    //             foundCount++;
    //         }
    //     }

    //     if (foundCount != 3 || dotIndex + 1 > routerAddress.getLength())
    //     {
    //         LOG_ERROR("IPv4 address is expected."
    //                   "(retval: %s foundCount:%d dotIndex:%d)",
    //                   *routerAddress, foundCount, dotIndex);
    //         return 1;
    //     }

    //     routerAddress.set(dotIndex + 1, '1');
    //     if (routerAddress.getLength() > dotIndex + 1)
    //     {
    //         routerAddress.setLength(dotIndex + 2);
    //     }
    //     LOG_VERBOSE("routerAddress is assumed at %s", *routerAddress);
    // }
    // SocketAddress socketAddress(*routerAddress);
    // ((NetworkInterface *)network)->add_dns_server(socketAddress);
    iotc_set_logging(IOTC_LOGGING_ALL);
    iotc_set_network_interface(network);

    int errorCode = iotc_init_context(&context);
    if (errorCode != 0)
    {
        LOG_ERROR("Error initializing IOTC. Code %d", errorCode);
        return 1;
    }

    //iotc_set_logging(IOTC_LOGGING_API_ONLY);

    // for the simplicity of this sample, used same callback for all the events below
    iotc_on(context, "MessageSent", onEvent, NULL);
    iotc_on(context, "Command", onEvent, NULL);
    iotc_on(context, "ConnectionStatus", onEvent, NULL);
    iotc_on(context, "SettingsUpdated", onEvent, NULL);
    iotc_on(context, "Error", onEvent, NULL);
    LOG_VERBOSE("Connecting to IoTCentral");
    //errorCode = iotc_connect(context, scopeId, connstring, deviceId, IOTC_CONNECT_CONNECTION_STRING);
    errorCode = iotc_connect(context, MBED_CONF_APP_SCOPEID, MBED_CONF_APP_DEVICEID, MBED_CONF_APP_DEVICEKEY, IOTC_CONNECT_SYMM_KEY);
    if (errorCode != 0)
    {
        LOG_ERROR("Error @ iotc_connect. Code %d", errorCode);
        return 1;
    }
    LOG_VERBOSE("----------------Client is connected --------------------------");
    prevMillis = us_ticker_read() / 1000;

    while (true)
        loop();

    return 1;
}
