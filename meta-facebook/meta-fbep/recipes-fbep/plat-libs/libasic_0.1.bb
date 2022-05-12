# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "OCP Accelerator module Library"
DESCRIPTION = "Library for communication with the ASIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://asic.c;beginline=8;endline=20;md5=df671c5f3a78585c16168736d9c3fc15"


SRC_URI = "file://asic \
          "
LDFLAGS += "-lobmc-i2c -lgpio-ctrl "
DEPENDS += "libobmc-i2c libgpio-ctrl "
RDEPENDS:${PN} += "libobmc-i2c "

S = "${WORKDIR}/asic"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libasic.so ${D}${libdir}/libasic.so

    install -d ${D}${includedir}/facebook
    install -m 0644 asic.h ${D}${includedir}/facebook/asic.h
}

FILES:${PN} = "${libdir}/libasic.so"
FILES:${PN}-dev = "${includedir}/facebook/asic.h"
