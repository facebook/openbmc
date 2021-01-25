# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "Helper Abstraction to the Intel MCTP Library"
DESCRIPTION = "Library for communication with the device which supported MCTP"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://obmc-mctp.h;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://obmc-mctp.c \
           file://obmc-mctp.h \
           file://Makefile \
          "

DEPENDS += "libncsi libpldm libmctp-intel"
RDEPENDS_${PN} += "libncsi libpldm libmctp-intel"
LDFLAGS += "-lpldm -lmctp_intel"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libobmc-mctp.so ${D}${libdir}/libobmc-mctp.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 obmc-mctp.h ${D}${includedir}/openbmc/obmc-mctp.h
}

FILES_${PN} = "${libdir}/libobmc-mctp.so"
FILES_${PN}-dev = "${includedir}/openbmc/obmc-mctp.h"
