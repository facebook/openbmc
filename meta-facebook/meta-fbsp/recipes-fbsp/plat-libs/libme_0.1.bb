# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "ME Library"
DESCRIPTION = "library for communicating with ME"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://me.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://me \
          "
DEPENDS += "libipmi libipmb"

S = "${WORKDIR}/me"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libme.so ${D}${libdir}/libme.so

    install -d ${D}${includedir}/facebook
    install -m 0644 me.h ${D}${includedir}/facebook/me.h
}

FILES_${PN} = "${libdir}/libme.so"
FILES_${PN}-dev = "${includedir}/facebook/me.h"
