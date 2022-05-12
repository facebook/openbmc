# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "MCTP Library"
DESCRIPTION = "library for communicating via MCTP Payload"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mctp.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://mctp \
          "

LDFLAGS += "-lobmc-i2c"
DEPENDS += "libobmc-i2c "
RDEPENDS:${PN} += "libobmc-i2c"

S = "${WORKDIR}/mctp"

do_install() {
	install -d ${D}${libdir}
    install -m 0644 libmctp.so ${D}${libdir}/libmctp.so

    install -d ${D}${includedir}/facebook
    install -m 0644 mctp.h ${D}${includedir}/facebook/mctp.h
}

FILES:${PN} += "${libdir}/libmctp.so"
FILES:${PN}-dev = "${includedir}/facebook/mctp.h"
