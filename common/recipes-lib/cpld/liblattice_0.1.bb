# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "CPLD Library"
DESCRIPTION = "Library for communicating with CPLD"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cpld.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://cpld.c \
           file://cpld.h \
           file://lattice.c \
           file://lattice.h \
           file://Makefile \
          "

DEPENDS = "libast-jtag"
RDEPENDS_${PN} = "libast-jtag"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 liblattice.so ${D}${libdir}/liblattice.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 cpld.h ${D}${includedir}/openbmc/cpld.h
}

FILES_${PN} = "${libdir}/liblattice.so"
FILES_${PN}-dev = "${includedir}/openbmc/cpld.h"
