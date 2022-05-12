# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBGC Common Library"
DESCRIPTION = "library for common fbgc information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fbgc_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

SRC_URI = "file://fbgc_common \
          "

DEPENDS += " python3-setuptools libobmc-i2c libgpio-ctrl libkv libfbgc-gpio libipmi openssl "
RDEPENDS:${PN} += "libobmc-i2c libgpio-ctrl libkv libfbgc-gpio libipmi openssl "

S = "${WORKDIR}/fbgc_common"

BBCLASSEXTEND = "native nativesdk"
