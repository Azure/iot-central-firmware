# ESP32/ESP8266 Azure IoT Central Demo

### Requirements

  - For ESP32 platform: [ESP-IDF](https://github.com/espressif/esp-idf)
  - For ESP8266 platform: [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK)

### Compiler

- For ESP32 platform: [Here](https://github.com/espressif/esp-idf/blob/master/README.md)
- For ESP8266 platform: [Here](https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/README.md)

## Configuring and Building

### Configuring your Azure IOT Hub Device Connection String, Wi-Fi and serial port

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (real device) (Under the `Device Explorer`) (Reason for `mxchip` selection is pre-populated template)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and use them on the next step
- `cd esp-azure`
- Go to `make menuconfig` -> `Azure IoT Central` to configure your Azure IOT Central Device credentials, Wi-Fi SSID and Password;
- Go to `make menuconfig` -> `Serial flasher config` to configure you serial port.

### Implementation details

See [esp-azure/main/azure-iot-central.c](./esp-azure/main/azure-iot-central.c) for IoTCentral specific connection and telemetry codes.
If you want to learn more advanced functionality, visit [../MXCHIP/mxchip_advanced/src/AzureIOTClient.cpp](../MXCHIP/mxchip_advanced/src/AzureIOTClient.cpp)

Both projects use `iotc` wrapper. You can use the IoTCentral related logic interchangebly on both side.

### Building your demo and flash to ESP device with `make flash`.

!!! (node-mcu) Press the button on the right if the process hangs on `connecting..`

If failed, please:

- make sure your ESP device had connected to PC with serial port;
- make sure you have selected the corrected serial port;
  - command `> sudo usermod -a -G dialout $USER` can also be used.

### To monitor the device output while running, run

``` bash
make monitor
```

To exit the monitor, hit Control-]

You can also run the build and monitor in onte step and run with multiple compiler threads:

``` bash
make -j4 flash monitor
```

This will build with four concurrent build processes to take advantage of more cores on your workstation.