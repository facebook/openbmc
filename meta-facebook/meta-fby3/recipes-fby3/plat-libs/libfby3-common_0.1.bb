# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBY3 Common Library"
DESCRIPTION = "library for common fby3 information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fby3_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fby3_common \
          "

CFLAGS += " -Wall -Werror -fPIC "
LDFLAGS = "-lobmc-i2c -lgpio-ctrl -lkv"
DEPENDS += "libobmc-i2c libgpio-ctrl libkv"

S = "${WORKDIR}/fby3_common"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libfby3_common.so ${D}${libdir}/libfby3_common.so

  install -d ${D}${includedir}
  install -d ${D}${includedir}/facebook
  install -m 0644 fby3_common.h ${D}${includedir}/facebook/fby3_common.h
}

FILES:${PN} = "${libdir}/libfby3_common.so"
FILES:${PN}-dev = "${includedir}/facebook/fby3_common.h"
