#
# Component Makefile
#

# Component configuration in preprocessor defines
CFLAGS += -DUSE_LWIP_SOCKET_FOR_AZURE_IOT -DHSM_TYPE_SYMM_KEY -DUSE_PROV_MODULE
CPPFLAGS += -DUSE_LWIP_SOCKET_FOR_AZURE_IOT -DHSM_TYPE_SYMM_KEY -DUSE_PROV_MODULE

COMPONENT_ADD_INCLUDEDIRS := \
. \
iotc

COMPONENT_OBJS = \
iotc/iotc.o \
iotc/common/parson.o \
iotc/common/iotc_common.o \
iotc/common/iotc_internal.o \
iotc/common/string_buffer.o \
azure_main.o \
azure-iot-central.o

COMPONENT_SRCDIRS := \
. \
iotc \
iotc/common

# COMPONENT_EMBED_TXTFILES := certs/leaf_private_key.pem certs/leaf_certificate.pem

# ifndef IDF_CI_BUILD
# # Print an error if the certificate/key files are missing
# $(COMPONENT_PATH)/certs/leaf_private_key.pem $(COMPONENT_PATH)/certs/leaf_certificate.pem:
# 	@echo "Missing PEM file $@. This file identifies the ESP32 to Azure DPS for the example, see README for details."
# 	exit 1
# else  # IDF_CI_BUILD
# # this case is for the internal Continuous Integration build which
# # compiles all examples. Add some dummy certs so the example can
# # compile (even though it won't work)
# $(COMPONENT_PATH)/certs/leaf_private_key.pem $(COMPONENT_PATH)/certs/leaf_certificate.pem:
# 	echo "Dummy certificate data for continuous integration" > $@
# endif

# CFLAGS += -DSET_TRUSTED_CERT_IN_SAMPLES
