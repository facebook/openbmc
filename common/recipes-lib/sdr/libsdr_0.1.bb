# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "SDR Library"
DESCRIPTION = "library for extracting information from SDR"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"

LIC_FILES_CHKSUM = "file://sdr.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

LOCAL_URI = " \
    file://meson.build\
    file://sdr.c \
    file://sdr.h \
    file://sdr.py \
    "

DEPENDS += " libipmi libpal"

inherit meson pkgconfig
inherit python3-dir

do_install:append() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/sdr.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}

FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}"
