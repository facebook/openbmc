# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "

LDFLAGS += " -lbic -lipmi -lipmb -lbic -lfbgc_gpio -lgpio -lpal"
DEPENDS += "libbic libfbgc-gpio libgpio libpal libipmi libipmb"
RDEPENDS_${PN} += "libbic libfbgc-gpio libgpio libpal libipmi libipmb"
