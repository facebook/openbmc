# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Lightning GPIO Library"
DESCRIPTION = "library for reading GPIO values"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://lightning_gpio.c;beginline=8;endline=20;md5=8ad57a98e852734170b0b3b1dbf2ac85"


SRC_URI = "file://lightning_gpio \
          "
DEPENDS += " liblightning-common libgpio"

S = "${WORKDIR}/lightning_gpio"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 liblightning_gpio.so ${D}${libdir}/liblightning_gpio.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 lightning_gpio.h ${D}${includedir}/facebook/lightning_gpio.h
}

FILES:${PN} = "${libdir}/liblightning_gpio.so"
FILES:${PN}-dev = "${includedir}/facebook/lightning_gpio.h"
RDEPENDS:${PN} += " liblightning-common libgpio"
