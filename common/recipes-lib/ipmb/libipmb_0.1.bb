# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "IPMB Client Library"
DESCRIPTION = "library for IPMB Client"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmb.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://ipmb.c \
           file://ipmb.h \
          "

S = "${WORKDIR}"

DEPENDS += "libipmi libipc"
RDEPENDS_${PN} += "libipmi libipc"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libipmb.so ${D}${libdir}/libipmb.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ipmb.h ${D}${includedir}/openbmc/ipmb.h
}

FILES_${PN} = "${libdir}/libipmb.so"
FILES_${PN}-dev = "${includedir}/openbmc/ipmb.h"
