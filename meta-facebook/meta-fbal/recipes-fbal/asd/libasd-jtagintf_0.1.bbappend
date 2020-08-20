# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "
LDFLAGS += "-lgpio-ctrl -lpal"
DEPENDS += "libpal libgpio-ctrl"
RDEPENDS_${PN} += "libpal libgpio-ctrl"

