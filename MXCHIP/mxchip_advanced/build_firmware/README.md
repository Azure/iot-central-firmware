### How to Build with MXchip source codes

If you want to edit base OS source code for the mxchip sample project and compile
that along with the example, please follow the steps below;

Clone `devkit-sdk` into this folder.
```
git clone https://github.com/Microsoft/devkit-sdk
cd devkit-sdk
git checkout 0ca1b6ed8d06e7a2b371eb2c4e54ff61d6e3e899
cd ..
```

Install [iotz](https://github.com/azure/iotz). You will need `Docker` and `node.js` (8+) is installed on your machine.
Don't forget sharing the host drive from `Docker Settings`->`File Sharing`

Once `iotz` is installed, `initialize` the project (only once and under the current project folder)
```
iotz init
```

and compile
```
iotz make
```

The resulting `iotCentral.ino.bin` file under the `../BUILD` folder is the firmware.
