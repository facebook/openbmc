# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Wedge EEPROM Library"
DESCRIPTION = "library for wedge eeprom"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://wedge_eeprom.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://lib \
          "

DEPENDS += "fbutils"

S = "${WORKDIR}/lib"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libwedge_eeprom.so ${D}${libdir}/libwedge_eeprom.so

    install -d ${D}${includedir}/facebook
    install -m 0644 wedge_eeprom.h ${D}${includedir}/facebook/wedge_eeprom.h
}

FILES_${PN} = "${libdir}/libwedge_eeprom.so"
FILES_${PN}-dbg = "${libdir}/.debug"
FILES_${PN}-dev = "${includedir}/facebook/wedge_eeprom.h"
