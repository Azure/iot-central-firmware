# ESP8266 basic example for Azure IoT Central

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `mxchip` device. (real device) (Under the `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `ESP8266.ino`

```
// #define WIFI_SSID "<ENTER WIFI SSID HERE>"
// #define WIFI_PASSWORD "<ENTER WIFI PASSWORD HERE>"

// const char* scopeId = "<ENTER SCOPE ID HERE>";
// const char* deviceId = "<ENTER DEVICE ID HERE>";
// const char* deviceKey = "<ENTER DEVICE primary/secondary KEY HERE>";
```

Compile it! and deploy to your device. (see below)

- Download Arduino-CLI from [this link](https://github.com/arduino/arduino-cli#download-the-latest-stable-release)
- Add additional url to board manager in .cli-config.yml (this is usually located in the same folder of CLI executable)

```
board_manager:
  additional_urls:
  - http://arduino.esp8266.com/stable/package_esp8266com_index.json
```
Setup the environment; (under the project folder)
```
arduino-cli-0.3.3 core update-index
arduino-cli-0.3.3 core install esp8266:esp8266
arduino-cli-0.3.3 board attach esp8266:esp8266:nodemcu
```

Compile!
```
arduino-cli-0.3.3 compile
```

Upload
```
arduino-cli-0.3.3 upload -p <PORT / DEV?? i.e. => /dev/cu.SLAB_USBtoUART >
```

Monitoring?

```
npm install -g nodemcu-tool
```

Assuming the port/dev for the board is `/dev/cu.SLAB_USBtoUART`
```
nodemcu-tool -p /dev/cu.SLAB_USBtoUART -b 9600 terminal
```
