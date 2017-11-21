# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "

CFLAGS += " -lbic -lipmi -lipmb -lbic -lfby2_gpio  "

DEPENDS += "libbic libfby2-gpio"
