# Copyright 2022-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://0001-netlakenext-dimm-modify-the-max-number-of-PMIC-error.patch \
            "

LOCAL_URI += " \
        file://plat/meson.build \
        file://dimm-functions.cpp \
        file://dimm-util-plat.h \
        file://dimm-pmic.cpp \
        "

DEPENDS += " libobmc-i2c libnetlakenext-common"
RDEPENDS:${PN} += "libnetlakenext-common"