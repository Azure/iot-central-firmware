## Azure IoT Central Reference Firmware for Raspberry Pi 2/3

### Description

An example of writing a firmware solution to send data to Azure IoT Central and
to receive events back from Azure IoT Central to be processed by the device.
You are free to take this code and the concepts used, and use them as a basis
for your own firmware for Azure IoT Central.

### How to run

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `Raspberry Pi` device. (real device) (Under the `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `mxchip_basic.ino`

Install `iotc` python client
```
pip install iotc
```

Go create an application
Fill the information below (under app.py).
```
deviceId = "DEVICE_ID"
scopeId = "SCOPE_ID"
mkey = "DEVICE_KEY"
```

Run!

```
python app.py
```

Python 2/3 and micropython(*) are supported.
