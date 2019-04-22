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

Compile it! and deploy to your device. No, make sure you have the [iot c sdk on your machine first](https://github.com/Azure/azure-iot-sdk-c), you must [setup your development environment on your machine](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/devbox_setup.md#macos).

You can use [Azure IoT Workbench](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/#install-development-environment)
or [iotz](https://github.com/Azure/iotz) to compile this sample

