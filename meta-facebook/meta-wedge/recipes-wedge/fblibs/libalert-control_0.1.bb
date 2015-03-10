# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Wedge Alert Control Library"
DESCRIPTION = "library for Wedge Alert Control"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://alert_control.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://alert_control \
          "

S = "${WORKDIR}/alert_control"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libalert_control.so ${D}${libdir}/libalert_control.so

    install -d ${D}${includedir}/facebook
    install -m 0644 alert_control.h ${D}${includedir}/facebook/alert_control.h
}

FILES_${PN} = "${libdir}/libalert_control.so"
FILES_${PN}-dbg = "${libdir}/.debug"
FILES_${PN}-dev = "${includedir}/facebook/alert_control.h"
