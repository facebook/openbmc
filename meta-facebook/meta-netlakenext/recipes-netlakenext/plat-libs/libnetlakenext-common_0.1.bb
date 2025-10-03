# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "NETLAKENEXT Common Library"
DESCRIPTION = "library for common netlakenext information"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://netlakenext_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

inherit meson

SRC_URI = "file://netlakenext_common \
          "

DEPENDS += " python3-setuptools libobmc-i2c libgpio-ctrl libkv libipmi libipmb openssl "
RDEPENDS_${PN} += "libobmc-i2c libgpio-ctrl libkv libipmi libipmb openssl "

S = "${WORKDIR}/netlakenext_common"

BBCLASSEXTEND = "native nativesdk"
