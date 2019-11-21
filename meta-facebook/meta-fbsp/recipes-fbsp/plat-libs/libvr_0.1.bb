# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Voltage Regulator Library"
DESCRIPTION = "Library for communication with the voltage regulator"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://vr.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://vr \
          "

LDFLAGS += "-lobmc-i2c -lkv"
DEPENDS += "libobmc-i2c libkv"
RDEPENDS_${PN} += "libobmc-i2c libkv"

S = "${WORKDIR}/vr"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libvr.so ${D}${libdir}/libvr.so

  install -d ${D}${includedir}/facebook
  install -m 0644 vr.h ${D}${includedir}/facebook/vr.h
}

FILES_${PN} = "${libdir}/libvr.so"
FILES_${PN}-dev = "${includedir}/facebook/vr.h"
