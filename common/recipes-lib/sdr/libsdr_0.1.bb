# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "SDR Library"
DESCRIPTION = "library for extracting information from SDR"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"


LIC_FILES_CHKSUM = "file://sdr.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

SRC_URI = "file://Makefile \
           file://sdr.c \
           file://sdr.h \
          "

S = "${WORKDIR}"

DEPENDS += " libipmi libpal "

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libsdr.so ${D}${libdir}/libsdr.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 sdr.h ${D}${includedir}/openbmc/sdr.h
}

FILES_${PN} = "${libdir}/libsdr.so"
FILES_${PN}-dev = "${includedir}/openbmc/sdr.h"
