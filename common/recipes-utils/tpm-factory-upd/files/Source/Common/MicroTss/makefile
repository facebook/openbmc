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
# Makefile to build the MicroTss library
#
# The makefile imports CC, CXX, CFLAGS etc. from its parent makefile
#

MAIN_TARGET=libmicrotss.a
OBJFILES=\
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
	TSC_PhysicalPresence.o

SRC_DIRS=\
	. \
	Tpm_1_2 \
	Tpm_2_0

INCLUDE_DIRS=\
	. \
	.. \
	../Crypt \
	../Platform \
	Tpm_1_2 \
	Tpm_2_0

INCLUDES=$(foreach d, $(INCLUDE_DIRS), -I$d)

.PHONY: all clean

vpath %.c $(SRC_DIRS)
vpath %.h $(INCLUDE_DIRS)

all: libmicrotss.a

$(OBJFILES): %.o: %.c
	$(CC) -c $(CFLAGS) $(FPACK) $(INCLUDES) $< -o $@

libmicrotss.a: $(OBJFILES)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -rfv *.o libmicrotss.a
