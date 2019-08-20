# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "ASPEED JTAG Library"
DESCRIPTION = "Library for communicating with ASPEED JTAG controller driver (kernel 5.0.3)"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ast-jtag.c;beginline=8;endline=20;md5=435eababecd3f367d90616c70e27bdd6"

SRC_URI = "file://ast-jtag.c \
           file://ast-jtag.h \
           file://Makefile \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libast-jtag.so ${D}${libdir}/libast-jtag.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ast-jtag.h ${D}${includedir}/openbmc/ast-jtag.h
}

FILES_${PN} = "${libdir}/libast-jtag.so"
FILES_${PN}-dev = "${includedir}/openbmc/ast-jtag.h"
