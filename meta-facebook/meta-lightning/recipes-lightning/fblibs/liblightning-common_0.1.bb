# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Lightning Common Library"
DESCRIPTION = "library for common Lightning information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://lightning_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS += "libkv"
RDEPENDS:${PN} += "libkv"

SRC_URI = "file://lightning_common \
          "

S = "${WORKDIR}/lightning_common"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 liblightning_common.so ${D}${libdir}/liblightning_common.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 lightning_common.h ${D}${includedir}/facebook/lightning_common.h
}

FILES:${PN} = "${libdir}/liblightning_common.so"
FILES:${PN}-dev = "${includedir}/facebook/lightning_common.h"
