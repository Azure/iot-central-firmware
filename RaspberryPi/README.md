## Azure IoT reference application for Raspberry Pi 2/3 (using python)

### Description

An example python app to..
- send data to Azure IoT Central
- receive events back from Azure IoT Central to be processed by the device.

### How to run

- Visit [AzureIoTCentral](https://apps.azureiotcentral.com) and create a `new application`.
- Select `Sample Devkits`
- Add a new `Raspberry Pi` device. (real device) (Under the `Device Explorer`)
- Browse into device window and click/open `Connect` on the top-right of the device screen
- Grab `scopeId`, `device Id` and `primary key` and fill the necessary parts under `app.py`

Install [azure iot central python client](https://pypi.org/project/iotc/) `iotc`
```
pip install iotc
```

Fill the information below (under app.py).
```
deviceId = "DEVICE_ID"
scopeId = "SCOPE_ID"
deviceKey = "DEVICE_KEY"
```

Run!

```
python app.py
```

Python 2/3 and micropython(*) are supported.


#### Using master key for producing deviceKey on runtime

Take a look at the `appMasterKey.py` sample
