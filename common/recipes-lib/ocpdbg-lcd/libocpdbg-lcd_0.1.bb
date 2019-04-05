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

LDFLAGS += " -lobmc-i2c"
DEPENDS = " libipmb libipmi libobmc-i2c"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libocpdbg-lcd.so ${D}${libdir}/libocpdbg-lcd.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ocp-dbg-lcd.h ${D}${includedir}/openbmc/ocp-dbg-lcd.h
}

FILES_${PN} = "${libdir}/libocpdbg-lcd.so"
FILES_${PN}-dev = "${includedir}/openbmc/ocp-dbg-lcd.h"
RDEPENDS_${PN} = "libipmb libipmi libobmc-i2c"
