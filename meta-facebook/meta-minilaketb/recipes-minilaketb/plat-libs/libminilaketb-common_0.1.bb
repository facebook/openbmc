# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "MINILAKETB Common Library"
DESCRIPTION = "library for common minilaketb information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://minilaketb_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://minilaketb_common \
          "

S = "${WORKDIR}/minilaketb_common"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libminilaketb_common.so ${D}${libdir}/libminilaketb_common.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 minilaketb_common.h ${D}${includedir}/facebook/minilaketb_common.h
}

FILES:${PN} = "${libdir}/libminilaketb_common.so"
FILES:${PN}-dev = "${includedir}/facebook/minilaketb_common.h"
