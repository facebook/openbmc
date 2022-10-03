# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "USB2JTAG Utility"
DESCRIPTION = "USB2JTAG Utility"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mpsse.c;beginline=4;endline=16;md5=ddec148a21e74d3e2d70c02da74399da"

inherit meson pkgconfig

SRC_URI = " file://libmpsse \
          "

def libftdi_option(d):
    distro = d.getVar('DISTRO_CODENAME', True)

    if distro in ['dunfell', 'kirkstone']:
        return "-Dlibftdi-1p4=enabled"
    return ""

DEPENDS += " libusb1 libftdi "
RDEPENDS:${PN} += " libusb1 libftdi "

S = "${WORKDIR}/libmpsse"

EXTRA_OEMESON = "${@libftdi_option(d)}"
