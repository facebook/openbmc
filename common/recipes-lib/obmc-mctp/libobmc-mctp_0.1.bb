# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "Helper Abstraction to the Intel MCTP Library"
DESCRIPTION = "Library for communication with the device which supported MCTP"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://obmc-mctp.h;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://obmc-mctp.c \
    file://obmc-mctp.h \
    file://meson.build \
    "

inherit meson pkgconfig

DEPENDS += "libncsi obmc-libpldm libmctp-intel"

