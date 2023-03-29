# Copyright 2021-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "

LDFLAGS += " -lbic -lipmi -lipmb -lbic -lfbgc_gpio -lgpio -lpal"
DEPENDS += "libbic libfbgc-gpio libgpio libpal libipmi libipmb"
