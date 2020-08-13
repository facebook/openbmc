# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "OCP Accelerator module Library"
DESCRIPTION = "Library for communication with the ASIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://asic.c;beginline=8;endline=20;md5=df671c5f3a78585c16168736d9c3fc15"


SRC_URI = "file://asic \
          "
LDFLAGS += "-lobmc-i2c -lgpio-ctrl -lkv"
DEPENDS += "libobmc-i2c libgpio-ctrl libkv"
RDEPENDS_${PN} += "libobmc-i2c libkv"

S = "${WORKDIR}/asic"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libasic.so ${D}${libdir}/libasic.so

    install -d ${D}${includedir}/facebook
    install -m 0644 asic.h ${D}${includedir}/facebook/asic.h
}

FILES_${PN} = "${libdir}/libasic.so"
FILES_${PN}-dev = "${includedir}/facebook/asic.h"
