# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "JTAG Interface library"
DESCRIPTION = "Library which helps interfacing with the JTAG hardware"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://daemon/LICENSE;beginline=4;endline=25;md5=8861f37222b237b82044f06478647f8b"

LOCAL_URI = " \
    file://daemon/asd_common.h \
    file://daemon/config.h \
    file://daemon/jtag_handler.c \
    file://daemon/jtag_handler.h \
    file://daemon/LICENSE \
    file://daemon/logging.c \
    file://daemon/logging.h \
    file://daemon/target_handler.h \
    file://daemon/tests/jtag.h \
    file://daemon/pin_handler.c \
    file://daemon/platform.c \
    file://meson.build \
    "

DEPENDS += "safec libgpio-ctrl"
RDEPENDS:${PN} += "safec libgpio-ctrl"

inherit meson pkgconfig
