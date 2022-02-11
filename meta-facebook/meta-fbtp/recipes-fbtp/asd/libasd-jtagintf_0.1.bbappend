# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "
LDFLAGS += "-lgpio-ctrl -lpal"
DEPENDS += "libpal libgpio-ctrl"
RDEPENDS:${PN} += "libpal libgpio-ctrl"

