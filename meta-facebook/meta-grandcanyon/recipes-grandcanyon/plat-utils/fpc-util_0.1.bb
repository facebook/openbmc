# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "Front Panel Control Utility"
DESCRIPTION = "Util to override the front panel control remotely"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fpc-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

inherit meson pkgconfig

SRC_URI = "file://fpc-util \
          "
S = "${WORKDIR}/fpc-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 fpc-util ${D}${bindir}/fpc-util
}

DEPENDS += " libpal libipmi libipmb "
RDEPENDS:${PN} += " libpal libipmi libipmb "

FILES:${PN} = "${bindir}"
