# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Yosemite Common Library"
DESCRIPTION = "library for common Yosemite information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fbttn_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fbttn_common \
          "

S = "${WORKDIR}/fbttn_common"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libfbttn_common.so ${D}${libdir}/libfbttn_common.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 fbttn_common.h ${D}${includedir}/facebook/fbttn_common.h
}

FILES:${PN} = "${libdir}/libfbttn_common.so"
FILES:${PN}-dev = "${includedir}/facebook/fbttn_common.h"
