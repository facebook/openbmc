# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Aries Retimer Library"
DESCRIPTION = "library for Aries Retimer v2.13"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=5;endline=17;md5=b96cf6516c0263b26b55c5bbf3806237"

LOCAL_URI = "\
    file://meson.build \
    file://aries_api.c \
    file://aries_i2c.c \
    file://aries_link.c \
    file://aries_margin.c \
    file://aries_misc.c \
    file://astera_log.c \
    file://include/aries_a0_reg_defines.h \
    file://include/aries_api.h \
    file://include/aries_api_types.h \
    file://include/aries_error.h \
    file://include/aries_globals.h \
    file://include/aries_i2c.h \
    file://include/aries_link.h \
    file://include/aries_margin.h \
    file://include/aries_misc.h \
    file://include/aries_mpw_reg_defines.h \
    file://include/astera_log.h \
    file://platform.c \
    file://include/platform.h \
    file://LICENSE \
    "

inherit meson pkgconfig

DEPENDS += " \
    libobmc-i2c \
    "

RDEPENDS:${PN} += " \
    libobmc-i2c \
    "
