# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "USB2JTAG Utility"
DESCRIPTION = "USB2JTAG Utility"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mpsse.c;beginline=4;endline=16;md5=ddec148a21e74d3e2d70c02da74399da"

inherit meson

SRC_URI = " file://libmpsse \
          "

DEPENDS += " libusb1 libftdi "
RDEPENDS_${PN} += " libusb1 libftdi "

S = "${WORKDIR}/libmpsse"
