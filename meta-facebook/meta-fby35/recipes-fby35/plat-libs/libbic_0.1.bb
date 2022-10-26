# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-or-later;md5=fed54355545ffd980b814dab4a3b312c"

inherit meson pkgconfig

SRC_URI = "file://bic"
S = "${WORKDIR}/bic"

DEPENDS += " \
    libfby35-common \
    libipmb \
    libipmi \
    libkv \
    libobmc-i2c \
    libusb1 \
    openssl \
"
RDEPENDS:${PN} += "libfby35-common"

