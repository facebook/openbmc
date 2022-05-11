# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Library"
DESCRIPTION = "library for communicating with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://bic.h \
    file://bic.c \
    file://bic_platform.h \
    file://bic_platform.c \
    file://meson.build \
    "

DEPENDS += "libmisc-utils libipmi libipmb libkv libobmc-i2c libgpio-ctrl"
RDEPENDS:${PN} += " libmisc-utils libobmc-i2c "


inherit meson pkgconfig
