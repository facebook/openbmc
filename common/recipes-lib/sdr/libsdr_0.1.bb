# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "SDR Library"
DESCRIPTION = "library for extracting information from SDR"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"

LIC_FILES_CHKSUM = "file://sdr.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

LOCAL_URI = " \
    file://Makefile \
    file://sdr.c \
    file://sdr.h \
    file://sdr.py \
    "

DEPENDS += " libipmi libpal"

inherit python3-dir

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libsdr.so ${D}${libdir}/libsdr.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 sdr.h ${D}${includedir}/openbmc/sdr.h
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/sdr.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}

FILES:${PN} = "${libdir}/libsdr.so \
                ${PYTHON_SITEPACKAGES_DIR}/sdr.py"
FILES:${PN}-dev = "${includedir}/openbmc/sdr.h"
