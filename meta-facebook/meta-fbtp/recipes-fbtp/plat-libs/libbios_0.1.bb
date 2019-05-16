# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "BIOS Library"
DESCRIPTION = "Library for manipulating BIOS"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bios.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://bios \
          "
DEPENDS += "libpal libgpio-ctrl"
RDEPENDS_${PN} += "libpal libgpio-ctrl"
LDFLAGS += "-lgpio-ctrl"

S = "${WORKDIR}/bios"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libbios.so ${D}${libdir}/libbios.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 bios.h ${D}${includedir}/openbmc/bios.h
}

FILES_${PN} = "${libdir}/libbios.so"
FILES_${PN}-dev = "${includedir}/openbmc/bios.h"
