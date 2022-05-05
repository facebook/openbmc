#
# Copyright 2015 - 2018 Infineon Technologies AG ( www.infineon.com )
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

# Export compiler flags to sub-makefiles
export CFLAGS
export STRIPFLAGS
export CC
export AR

LDFLAGS+=\
	-lcryptifx -L../Common/Crypt \
	-lplatform -L../Common/Platform \
	-lmicrotss -L../Common/MicroTss \
	-lfileio -L../Common/FileIO \
	-ltpmdeviceaccess -L../Common/TpmDeviceAccess \
	-lconsoleio -L../Common/ConsoleIO \

ifneq ($(and $(CROSS_COMPILE),$(STAGING_DIR),$(TARGET_NAME)),)
CROSS_INCLUDE_DIR=$(STAGING_DIR)/$(TARGET_NAME)/usr/include
export CROSS_INCLUDE_DIR
LDFLAGS+= -lcrypto -L$(STAGING_DIR)/$(TARGET_NAME)/usr/lib
else
LDFLAGS+= -lcrypto
endif

MAIN_TARGET=TPMFactoryUpd
OBJFILES=\
	TPMFactoryUpd.o \
	CommandFlow_Init.o \
	CommandFlow_TpmInfo.o \
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
	Utility.o

SRC_DIRS=\
	. \
	./Linux \
	../Common \

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
	# Call shared sub-makefiles to generate archives
	$(MAKE) -C ../Common/Platform
	$(MAKE) -C ../Common/ConsoleIO
	$(MAKE) -C ../Common/MicroTss
	$(MAKE) -C ../Common/FileIO
	$(MAKE) -C ../Common/TpmDeviceAccess
	$(MAKE) -C ../Common/Crypt
	# And run the actual makefile job
	$(CC) $^ -o $@ $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS)

clean:
	# Call shared sub-makefiles to cleanup archives
	$(MAKE) -C ../Common/Platform clean
	$(MAKE) -C ../Common/ConsoleIO clean
	$(MAKE) -C ../Common/MicroTss clean
	$(MAKE) -C ../Common/FileIO clean
	$(MAKE) -C ../Common/TpmDeviceAccess clean
	$(MAKE) -C ../Common/Crypt clean
	# And clean everything for the actual makefile
	rm -rfv *.o TPMFactoryUpd

