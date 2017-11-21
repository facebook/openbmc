# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBY2 Common Library"
DESCRIPTION = "library for common fby2 information"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fby2_common.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fby2_common \
          "

S = "${WORKDIR}/fby2_common"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libfby2_common.so ${D}${libdir}/libfby2_common.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 fby2_common.h ${D}${includedir}/facebook/fby2_common.h
}

FILES_${PN} = "${libdir}/libfby2_common.so"
FILES_${PN}-dev = "${includedir}/facebook/fby2_common.h"
