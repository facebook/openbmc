# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic.h;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

inherit meson pkgconfig

SRC_URI = "file://bic \
          "

DEPENDS += "libipmi libipmb libobmc-i2c libgpio-ctrl libfby3-common libkv libusb1 libfby3-common openssl"
RDEPENDS:${PN} += "libfby3-common"

S = "${WORKDIR}/bic"
