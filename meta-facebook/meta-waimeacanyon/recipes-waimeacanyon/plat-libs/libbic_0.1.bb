# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBWC Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic_xfer.h;beginline=1;endline=19;md5=5301435cd2b862fa7dd98c513f7dfe9c"

inherit meson pkgconfig

SRC_URI = "file://bic \
          "

LDFLAGS = "-lusb-1.0"

DEPENDS += " python3-setuptools libobmc-i2c libgpio-ctrl libkv libipmb libfbwc-common libusb1 openssl"

S = "${WORKDIR}/bic"

BBCLASSEXTEND = "native nativesdk"
