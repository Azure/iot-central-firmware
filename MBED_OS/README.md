# MBED OS 5.X+ basic example for Azure IoT Central

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (real device) (Under the `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `app_main.cpp`

```
// #define WIFI_SSID "<ENTER WIFI SSID HERE>"
// #define WIFI_PASSWORD "<ENTER WIFI PASSWORD HERE>"

// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";
```

Create a `src/` folder and copy `../iotc` folder into it.

Compile it! and deploy to your device.

- Use MBED online compiler

OR

- Install iotz from [this link](https://github.com/Azure/iotz)
- - (update board name under `iotz.json` i.e. currently it's `"target": "nucleo_l476rg"`)

Setup the environment; (under the project folder)
```
iotz init
iotz export
```

Compile!
```
iotz make -j2
```

Upload
```
Copy the `.bin` file under the `BUILD` folder into your device (possibly an attached disk drive)
```

Monitoring?

```
npm install -g nodemcu-tool
```

Assuming the port/dev for the board is `/dev/tty.usbxxxxxxx`
```
nodemcu-tool -p /dev/tty.usbxxxxx -b 9600 terminal
```
