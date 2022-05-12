# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Expander Library"
DESCRIPTION = "library for communicating with Expander"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://exp.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://exp \
          "
DEPENDS += "libipmi libipmb fbutils"

S = "${WORKDIR}/exp"

do_install() {
	install -d ${D}${libdir}
    install -m 0644 libexp.so ${D}${libdir}/libexp.so

    install -d ${D}${includedir}/facebook
    install -m 0644 exp.h ${D}${includedir}/facebook/exp.h
}

FILES:${PN} += "${libdir}/libexp.so"
FILES:${PN}-dev = "${includedir}/facebook/exp.h"
