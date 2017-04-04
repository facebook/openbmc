# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing common features for the LCD USB debug card"
DESCRIPTION = "Library providing common features for the LCD USB debug card"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ocp-dbg-lcd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://ocp-dbg-lcd.c \
           file://ocp-dbg-lcd.h \
          "

S = "${WORKDIR}"

DEPENDS = " libipmb libipmi "

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libocp-dbg-lcd.so ${D}${libdir}/libocp-dbg-lcd.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ocp-dbg-lcd.h ${D}${includedir}/openbmc/ocp-dbg-lcd.h
}

FILES_${PN} = "${libdir}/libocp-dbg-lcd.so"
FILES_${PN}-dev = "${includedir}/openbmc/ocp-dbg-lcd.h"

PROVIDES = "libocp-dbg-lcd"
