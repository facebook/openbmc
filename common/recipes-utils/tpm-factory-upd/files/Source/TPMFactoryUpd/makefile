#
# Copyright 2015 - 2022 Infineon Technologies AG ( www.infineon.com )
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Makefile to build the TPMFactoryUpd application
#
# The makefile uses the gcc compiler.
#
CC=$(CROSS_COMPILE)gcc
STRIP=$(CROSS_COMPILE)strip
AR=$(CROSS_COMPILE)ar
#FPACK+= -fpack-struct # Don't pack structs globally. This would crash OpenSSL decrypt operation

# Basic compiler options
CFLAGS+= \
	-Wall \
	-Wextra \
	-std=gnu1x -Wpedantic \
	-Wno-missing-field-initializers \
	-Wno-error=deprecated-declarations \
	-Werror \
	-Wshadow \
	-Wcast-align \
	-Wswitch-default \
	-Wunreachable-code \
	-Wno-implicit-fallthrough \
	-DLINUX \
	-D_FORTIFY_SOURCE=1 \
	-fstack-protector-all

STRIPFLAGS+= --strip-unneeded $@  # Don't strip if you want to debug

ifneq ($(and $(CROSS_COMPILE),$(STAGING_DIR),$(TARGET_NAME)),)
CROSS_INCLUDE_DIR=$(STAGING_DIR)/$(TARGET_NAME)/usr/include
LDFLAGS+= -lcrypto -L$(STAGING_DIR)/$(TARGET_NAME)/usr/lib
else
LDFLAGS+= -lcrypto
endif

MAIN_TARGET=TPMFactoryUpd
OBJFILES=\
	TPMFactoryUpd.o \
	CommandFlow_Init.o \
	CommandFlow_TpmInfo.o \
	CommandFlow_SetMode.o \
	CommandFlow_TpmUpdate.o \
	CommandFlow_Tpm12ClearOwnership.o \
	CommandLineParser.o \
	CommandLine.o \
	Config.o \
	ConfigSettings.o \
	Controller.o \
	ControllerCommon.o \
	DeviceManagement.o \
	Error.o \
	FirmwareImage.o \
	FirmwareUpdate.o \
	Logging.o \
	PropertyStorage.o \
	Response.o \
	TpmResponse.o \
	UiUtility.o \
	Utility.o \
	Common.o \
	ConsoleIO.o \
	Crypt.o \
	FileIO.o \
	TPM_FieldUpgradeComplete.o \
	TPM_FieldUpgradeInfoRequest.o \
	TPM_FieldUpgradeInfoRequest2.o \
	TPM_FieldUpgradeStart.o \
	TPM_FieldUpgradeUpdate.o \
	TPM_FlushSpecific.o \
	TPM_GetCapability.o \
	TPM_GetTestResult.o \
	TPM_Marshal.o \
	TPM_OIAP.o \
	TPM_OSAP.o \
	TPM_OwnerClear.o \
	TPM_OwnerReadInternalPub.o \
	TPM_ReadPubEK.o \
	TPM_SetCapability.o \
	TPM_Startup.o \
	TPM_TakeOwnership.o \
	TPM2_FieldUpgradeAbandonVendor.o \
	TPM2_FieldUpgradeDataVendor.o \
	TPM2_FieldUpgradeFinalizeVendor.o \
	TPM2_FieldUpgradeManifestVendor.o \
	TPM2_FieldUpgradeMarshal.o \
	TPM2_FieldUpgradeStartVendor.o \
	TPM2_FlushContext.o \
	TPM2_GetCapability.o \
	TPM2_GetTestResult.o \
	TPM2_HierarchyChangeAuth.o \
	TPM2_Marshal.o \
	TPM2_PolicyCommandCode.o \
	TPM2_PolicySecret.o \
	TPM2_SetPrimaryPolicy.o \
	TPM2_Shutdown.o \
	TPM2_StartAuthSession.o \
	TPM2_Startup.o \
	TPM2_VendorMarshal.o \
	TSC_PhysicalPresence.o \
	Platform.o \
	DeviceAccess.o \
	DeviceAccessTpmDriver.o \
	TPM_TIS.o \
	TpmIO.o

SRC_DIRS=\
	. \
	./Linux \
	../Common \
	../Common/ConsoleIO \
	../Common/ConsoleIO/Linux \
	../Common/Crypt/Linux \
	../Common/FileIO/Linux \
	../Common/MicroTss/Tpm_1_2 \
	../Common/MicroTss/Tpm_2_0 \
	../Common/Platform/Linux \
	../Common/TpmDeviceAccess \
	../Common/TpmDeviceAccess/Linux \

INCLUDE_DIRS=\
	. \
	../Common \
	../Common/ConsoleIO \
	../Common/Crypt \
	../Common/FileIO \
	../Common/MicroTss \
	../Common/MicroTss/Tpm_1_2 \
	../Common/MicroTss/Tpm_2_0 \
	../Common/Platform \
	../Common/TpmDeviceAccess \
	../IFXTPMUpdate/Linux

INCLUDE_DIRS+=$(CROSS_INCLUDE_DIR)

INCLUDES=$(foreach d, $(INCLUDE_DIRS), -I$d)

.PHONY: all clean debug

vpath %.c $(SRC_DIRS)
vpath %.h $(INCLUDE_DIRS)

all: TPMFactoryUpd

debug: CFLAGS+=-DDEBUG -g
debug: STRIP=
debug: STRIPFLAGS=
debug: TPMFactoryUpd

coverage: CFLAGS+=-fprofile-arcs -ftest-coverage
coverage: LDFLAGS+=--coverage
coverage: TPMFactoryUpd

$(OBJFILES): %.o: %.c
	$(CC) -c $(CFLAGS) $(FPACK) $(INCLUDES) $< -o $@

TPMFactoryUpd: $(OBJFILES)
	$(CC) $^ -o $@ $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS)

clean:
	rm -rfv *.o TPMFactoryUpd

