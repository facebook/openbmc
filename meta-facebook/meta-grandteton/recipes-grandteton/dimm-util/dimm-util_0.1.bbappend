# Copyright 2022-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
        file://me-functions.cpp \
        file://dimm-util-plat.h \
        "

LDFLAGS += "-lipmb -lnm"

DEPENDS += "libipmi libipmb libnm"
RDEPENDS:${PN} += "libipmb libnm"
