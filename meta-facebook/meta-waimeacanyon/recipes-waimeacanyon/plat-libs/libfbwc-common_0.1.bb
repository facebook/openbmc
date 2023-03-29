# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBWC Common Library"
DESCRIPTION = "library for common fbwc information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fbwc_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

SRC_URI = "file://fbwc_common \
          "

DEPENDS += " python3-setuptools libobmc-i2c libgpio-ctrl libkv libipmi openssl "
RDEPENDS:${PN} += "openssl "

S = "${WORKDIR}/fbwc_common"

BBCLASSEXTEND = "native nativesdk"
