# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Simple Mocking for C-unit tests"
DESCRIPTION = "Headers to perform simple Mocking for C unit tests"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cmock.h;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://cmock.h \
          "
BBCLASSEXTEND = "native"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${includedir}/openbmc
    install -m 0644 cmock.h ${D}${includedir}/openbmc/cmock.h
}

