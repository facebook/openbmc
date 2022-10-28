# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBY35 Common Library"
DESCRIPTION = "library for common fby35 information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fby35_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://fby35_common"
S = "${WORKDIR}/fby35_common"

inherit meson pkgconfig

DEPENDS += "libobmc-i2c libgpio-ctrl libkv openssl"

