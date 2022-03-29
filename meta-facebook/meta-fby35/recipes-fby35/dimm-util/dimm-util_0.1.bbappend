# Copyright 2022-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
        file://me-functions.cpp \
        file://dimm-util-plat.h \
        "

LDFLAGS += "-lbic"

DEPENDS += "libbic"
RDEPENDS:${PN} += "libbic"
