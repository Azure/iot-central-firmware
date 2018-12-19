# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

PROJECT_NAME := azure_iot_central

ifeq ($(OS),Windows_NT)
$(shell copy ..\..\MXCHIP\mxchip_basic\src\iotc\*.* main\iotc)
else
$(shell cp -f ../../MXCHIP/mxchip_basic/src/iotc/*.* main/iotc)
endif

include $(IDF_PATH)/make/project.mk
