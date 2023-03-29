# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
            file://interface/jtag_handler.h \
            file://interface/jtag.h \
           "
LDFLAGS += "-lgpio-ctrl -lpal"
DEPENDS += "libpal libgpio-ctrl"
