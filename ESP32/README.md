# ESP32/ESP8266 Azure IoT Central Demo

## Prerequisites

Identify whether your device uses ESP32 or ESP8266 and follow the applicable instructions to set up ESP-IDF (Espressif IoT Development Framework):

  - For ESP32 platform: [ESP-IDF](https://github.com/espressif/esp-idf)
  - For ESP8266 platform: [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK)

## Configuring and Building

### Configuring your Azure IOT Hub Device Connection String, Wi-Fi and serial port

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (real device) (Under `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and use them on the next step

On your machine, open a terminal and navigate to the ESP32 sample directory, then run the following commands:

``` bash
cd esp-azure
make menuconfig
```

You will see the ESP-IDF configuration interface.

- Go to `Azure IoT Central Configuration` and fill in the Azure IoT device credentials from earlier, as well as your Wi-Fi SSID and Password
- Go to `Serial flasher config` to configure specify the serial port to which your device is connected
  - *Note: If you are using the Linux subsystem on Windows, the serial port will be `/dev/ttySN`, where **N** is the number of the `COM` port. For example, if your device is connected to `COM5`, the serial port will be `/dev/tty/S5`.

### Building and flashing to the ESP device.

``` bash
make flash
```

**For Node-MCU: Press the button on the right if the process hangs on `connecting..`**

To monitor the device output while running:

``` bash
make monitor
```

You can also run the build and monitor in onte step and run with multiple compiler threads:

``` bash
make -j4 flash monitor
```

This will build with four concurrent build processes to take advantage of more cores on your workstation.

To exit the monitor, hit Control-]

### Troubleshooting:

- Make sure your ESP device had connected to PC with serial port;
- Make sure you have selected the corrected serial port;
  - command `> sudo usermod -a -G dialout $USER` can also be used.

### Implementation details

See [esp-azure/main/azure-iot-central.c](./esp-azure/main/azure-iot-central.c) for IoTCentral specific connection and telemetry codes.
If you want to learn more advanced functionality, visit [../MXCHIP/mxchip_advanced/src/AzureIOTClient.cpp](../MXCHIP/mxchip_advanced/src/AzureIOTClient.cpp)

Both projects use `iotc` wrapper. You can use the IoTCentral related logic interchangebly on both side.
