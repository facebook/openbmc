# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Aries Retimer Library"
DESCRIPTION = "library for Aries Retimer v2.16.2"
SECTION = "base"
PR = "r1"
DEFAULT_PREFERENCE = "-1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${UNPACKDIR}/LICENSE;beginline=5;endline=17;md5=b96cf6516c0263b26b55c5bbf3806237"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = "\
    file://aries_api.c \
    file://aries_i2c.c \
    file://aries_link.c \
    file://aries_margin.c \
    file://aries_misc.c \
    file://astera_log.c \
    file://aries_a0_reg_defines.h \
    file://aries_api.h \
    file://aries_api_types.h \
    file://aries_error.h \
    file://aries_globals.h \
    file://aries_i2c.h \
    file://aries_link.h \
    file://aries_margin.h \
    file://aries_misc.h \
    file://aries_mpw_reg_defines.h \
    file://astera_log.h \
    file://LICENSE \
    file://meson.build \
    file://platform.c \
    file://platform.h \
    file://aries_common.c \
    file://aries_common.h \
    file://plat/meson.build \
    file://plat.c \
    file://plat.h \
    "

inherit meson pkgconfig

DEPENDS += " \
    libobmc-i2c \
    "

RDEPENDS:${PN} += " \
    libobmc-i2c \
    "
