# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBY35 Common Library"
DESCRIPTION = "library for common fby35 information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fby35_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fby35_common \
          "

CFLAGS += " -Wall -Werror -fPIC "
LDFLAGS = "-lobmc-i2c -lgpio-ctrl -lkv"
DEPENDS += "libobmc-i2c libgpio-ctrl libkv openssl"
RDEPENDS:${PN} += "libobmc-i2c libgpio-ctrl libkv openssl"

S = "${WORKDIR}/fby35_common"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libfby35_common.so ${D}${libdir}/libfby35_common.so

  install -d ${D}${includedir}
  install -d ${D}${includedir}/facebook
  install -m 0644 fby35_common.h ${D}${includedir}/facebook/fby35_common.h
}

FILES:${PN} = "${libdir}/libfby35_common.so"
FILES:${PN}-dev = "${includedir}/facebook/fby35_common.h"
