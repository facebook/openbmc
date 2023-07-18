# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "JTAG Interface library"
DESCRIPTION = "Library which helps interfacing with the JTAG hardware"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=25;md5=8861f37222b237b82044f06478647f8b"

LOCAL_URI = " \
    file://include/asd_common.h \
    file://include/config.h \
    file://include/logging.h \
    file://server/logging.c \
    file://target/jtag_handler.c \
    file://target/jtag_handler.h \
    file://target/target_handler.h \
    file://target/pin_handler.c \
    file://target/platform.c \
    file://target/tests/jtag.h \
    file://LICENSE \
    file://meson.build \
    "

DEPENDS += "safec libgpio-ctrl"
RDEPENDS:${PN} += "safec libgpio-ctrl"

inherit meson pkgconfig
