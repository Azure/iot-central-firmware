# Microsoft IoT Central Reference Firmware for Raspberry Pi 2/3

## Description:

Coming soon

## Installing:

- Setup your Raspberry Pi 2/3 with the latest Raspian release ( https://www.raspberrypi.org/downloads/raspbian/ ) and get it  connected to the internet via ethernet or wifi.  You will need an HDMI monitor, mouse and keyboard for this part.

- If you have a Pi Sense Hat ( https://www.raspberrypi.org/products/sense-hat/ ) install it on your Raspberry Pi.  If you do not no worries you can use the Pi Sense Hat software emulator on your Raspberry Pi OS.

- From the web browser on your Raspbarry Pi go to our GitHub releases ( https://github.com/Microsoft/microsoft-iot-central-firmware/releases ) and download the latest release named RaspberryPi-IoT-Central-X.X.X.zip (replace X with the latest version number).

- Open the file manager and find the file you just downloaded (it will be in the Downloads folder).  Move it up to the the root of your home directory /home/pi then right click the file and choose "Extract Here".  The file will be extracted into a directory called IoT-Central.

- If you do not have a Pi Sense Hat and want to use the emulator you need to edit the config.iot file.  Right click it in the file manager and choose "Text Editor".  Find the line "SimulateSenseHat" : false and change the false to true.

- Open a terminal window and use the command:
```
    cd /home/pi/IoT-Central
```

- In the same terminal window use the command:
```
    ./start.sh
```

- You may be prompted for the admin users password if you have changed it to install some dependencies.

- If you are using the sense hat emulator it will start up in a window. And the IoT Central agent will start.

- Open a browser and go to http://localhost:8080/start and configure your device with a connection string and choose the telemetry values you want to be sent to IoT Central.

- Click the "Configure Device" button and the device will start sending telemetry to IoT Central.


## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
