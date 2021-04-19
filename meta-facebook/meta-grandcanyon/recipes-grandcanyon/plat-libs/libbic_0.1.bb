# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBGC Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic.h;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

inherit meson

SRC_URI = "file://bic \
          "

DEPENDS += " python3-setuptools libobmc-i2c libgpio-ctrl libkv libipmb libfbgc-common libusb1"
RDEPENDS_${PN} += "libobmc-i2c libgpio-ctrl libkv libipmb libfbgc-common libusb1"

S = "${WORKDIR}/bic"

BBCLASSEXTEND = "native nativesdk"
