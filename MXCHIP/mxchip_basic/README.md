# MXCHIP basic example for Azure IoT Central

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (a real device) (under `Device Explorer`)
- Browse into device UI (by clicking to name of the device under `Device explorer`)
- click/open `Connect` at top-right of the device UI
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `mxchip_basic.ino`

```
// IOTConnectType connectType = IOTC_CONNECT_SYMM_KEY;
// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";
```

Make sure have an Azure IoT SDK to compile successfully. Fro example I used an [Arduino](https://www.arduino.cc/en/main/software#download) development environment with the following Arduino Libraries: [AzureIoTUtility](https://github.com/Azure/azure-iot-arduino-utility), [AzureIoTProtocol_MQTT](https://github.com/Azure/azure-iot-arduino-protocol-mqtt), [AzureIoTProtocol_HTTP](https://github.com/Azure/azure-iot-arduino-protocol-http) all at version `1.0.45`.

Compile it! and deploy to your device.

It's recomened you use [Visual Studio Code extension for Arduino](https://github.com/Microsoft/vscode-arduino), install it from [here](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino). You can also use [Azure IoT Workbench](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/#install-development-environment)
or [iotz](https://github.com/Azure/iotz) to compile this sample.

## Consider changing the IoT Central Ranges
Consider changes the data ranges for your plots in IoT Central, this will vary depending on sensor input data and if you decide to use the onboard sensors or conenct other sensors to the board. For my device template I used these ranges.
* `accelerometerX` Minimum Value: -50 Maximum Value: 50
* `accelerometerY` Minimum Value: -50 Maximum Value: 50
* `accelerometerZ` Minimum Value: -50 Maximum Value: 50
* `gyroscopeX` Minimum Value: -2000 Maximum Value: 1000
* `gyroscopeY` Minimum Value: -2000 Maximum Value: 1000
* `gyroscopeZ` Minimum Value: -2000 Maximum Value: 1000
* `temperature` Minimum Value: -40 Maximum Value: 120
* `humidity` Minimum Value: 10 Maximum Value: 80
