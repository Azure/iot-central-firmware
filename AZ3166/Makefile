
# ----------------------------------------------------------------------------
#  Copyright (C) Microsoft. All rights reserved.
#  Licensed under the MIT license.
# ----------------------------------------------------------------------------

all:
	rm -rf BUILD/build.options.json
	cp mxchip_tweak .iotz.mxchip.tweak
	rm /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/system/sdk/lib/libaz_iot.a
	rm -rf /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/cores/arduino/az_iot
	rm -rf /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/libraries/AzureIoT
	ln -s `pwd`/devkit-sdk/AZ3166/src/libraries/AzureIoT /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/libraries/AzureIoT
	ln -s `pwd`/devkit-sdk/AZ3166/src/cores/arduino/az_iot /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/cores/arduino/az_iot
	cp `pwd`/devkit-sdk/AZ3166/src/cores/arduino/EEPROMInterface.h /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/cores/arduino/
	cp .iotz.mxchip.tweak /root/.arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/platform.txt
	arduino --board 'AZ3166:stm32f4:MXCHIP_AZ3166' --verify 'iotCentral.ino' --pref build.path=/src/iot-central-firmware/AZ3166/BUILD && \
	cd /root/.arduino15/packages/AZ3166/hardware/stm32f4/ && cd `ls | awk '{print $1}'` && \
	cp bootloader/boot.bin /tools && \
	mv /src/iot-central-firmware/AZ3166/BUILD/iotCentral.ino.bin /src/iot-central-firmware/AZ3166/BUILD/iotCentral.ino_no_bootloader.bin && \
	python /tools/az3166_boot_patch.py /src/iot-central-firmware/AZ3166/BUILD/iotCentral.ino_no_bootloader.bin /src/iot-central-firmware/AZ3166/BUILD/iotCentral.ino.bin
clean :
	rm -rf BUILD/
