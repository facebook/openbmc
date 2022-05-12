# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Lightning Sensor Library"
DESCRIPTION = "library for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://lightning_sensor.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://lightning_sensor \
          "
LDFLAGS += "-lobmc-i2c"
DEPENDS += " libipmi liblightning-common liblightning-flash liblightning-gpio libobmc-i2c obmc-pal"

S = "${WORKDIR}/lightning_sensor"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 liblightning_sensor.so ${D}${libdir}/liblightning_sensor.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 lightning_sensor.h ${D}${includedir}/facebook/lightning_sensor.h
}

FILES:${PN} = "${libdir}/liblightning_sensor.so"
FILES:${PN}-dev = "${includedir}/facebook/lightning_sensor.h"
RDEPENDS:${PN} += " libipmi liblightning-common liblightning-flash liblightning-gpio libobmc-i2c"
