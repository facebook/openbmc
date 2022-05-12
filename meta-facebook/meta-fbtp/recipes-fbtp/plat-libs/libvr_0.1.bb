# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Voltage Regulator Library"
DESCRIPTION = "Library for communication with the voltage regulator"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://vr.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://vr \
          "
LDFLAGS += "-lobmc-i2c"
DEPENDS += "libobmc-i2c libkv"
RDEPENDS:${PN} += "libobmc-i2c libkv"

S = "${WORKDIR}/vr"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libvr.so ${D}${libdir}/libvr.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 vr.h ${D}${includedir}/openbmc/vr.h
}

FILES:${PN} = "${libdir}/libvr.so"
FILES:${PN}-dev = "${includedir}/openbmc/vr.h"
