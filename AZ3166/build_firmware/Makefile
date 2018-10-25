# ----------------------------------------------------------------------------
#  Copyright (C) Microsoft. All rights reserved.
#  Licensed under the MIT license.
# ----------------------------------------------------------------------------

all:
	SDK_DIR=`ls /root/.arduino15/packages/AZ3166/hardware/stm32f4/ | awk '{print $1}'` && \
	SDK_DIR="/root/.arduino15/packages/AZ3166/hardware/stm32f4/$$SDK_DIR" && \
	cp mxchip_tweak ../.iotz.mxchip.tweak && \
	cd .. && \
	rm -rf BUILD/build.options.json && \
	rm -f $$SDK_DIR/system/sdk/lib/libaz_iot.a && \
	rm -rf $$SDK_DIR/cores/arduino/az_iot && \
	rm -rf $$SDK_DIR/libraries/AzureIoT && \
	ln -s `pwd`/build_firmware/devkit-sdk/AZ3166/src/libraries/AzureIoT $$SDK_DIR/libraries/AzureIoT && \
	ln -s `pwd`/build_firmware/devkit-sdk/AZ3166/src/cores/arduino/az_iot $$SDK_DIR/cores/arduino/az_iot && \
	cp `pwd`/build_firmware/devkit-sdk/AZ3166/src/cores/arduino/EEPROMInterface.h $$SDK_DIR/cores/arduino/ && \
	cp .iotz.mxchip.tweak $$SDK_DIR/platform.txt && \
	arduino --board 'AZ3166:stm32f4:MXCHIP_AZ3166' --verify 'iotCentral.ino' --pref build.path=BUILD && \
	cp $$SDK_DIR/bootloader/boot.bin /tools && \
	mv BUILD/iotCentral.ino.bin BUILD/iotCentral.ino_no_bootloader.bin && \
	python /tools/arduino.az3166.boot_patch.py BUILD/iotCentral.ino_no_bootloader.bin BUILD/iotCentral.ino.bin && echo "DONE!"
clean :
	rm -rf BUILD/
