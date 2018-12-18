# MXCHIP basic example for Azure IoT Central

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (real device) (Under the `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `mxchip_basic.ino`

```
// IOTConnectType connectType = IOTC_CONNECT_SYMM_KEY;
// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";
```

Compile it! and deploy to your device.

You can use [Azure IoT Workbench](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/#install-development-environment)
or [iotz](https://github.com/Azure/iotz) to compile this sample

