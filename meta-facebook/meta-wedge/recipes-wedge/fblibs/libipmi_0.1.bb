# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Wedge IPMI Client Library"
DESCRIPTION = "library for Wedge IPMI Client"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmi.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://ipmi \
          "

S = "${WORKDIR}/ipmi"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libipmi.so ${D}${libdir}/libipmi.so

    install -d ${D}${includedir}/facebook
    install -m 0644 ipmi.h ${D}${includedir}/facebook/ipmi.h
}

FILES_${PN} = "${libdir}/libipmi.so"
FILES_${PN}-dbg = "${libdir}/.debug"
FILES_${PN}-dev = "${includedir}/facebook/ipmi.h"
