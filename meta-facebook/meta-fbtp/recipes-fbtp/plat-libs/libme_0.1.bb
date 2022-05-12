# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "ME Library"
DESCRIPTION = "library for communicating with ME"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://me.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://me \
          "
DEPENDS += "libipmi libipmb"

S = "${WORKDIR}/me"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libme.so ${D}${libdir}/libme.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 me.h ${D}${includedir}/openbmc/me.h
}

FILES:${PN} = "${libdir}/libme.so"
FILES:${PN}-dev = "${includedir}/openbmc/me.h"
