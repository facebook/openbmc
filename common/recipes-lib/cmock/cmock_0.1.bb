# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Simple Mocking for C-unit tests"
DESCRIPTION = "Headers to perform simple Mocking for C unit tests"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://cmock.h;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://cmock.h \
    "

BBCLASSEXTEND = "native"

do_install() {
    install -d ${D}${includedir}/openbmc
    install -m 0644 cmock.h ${D}${includedir}/openbmc/cmock.h
}

