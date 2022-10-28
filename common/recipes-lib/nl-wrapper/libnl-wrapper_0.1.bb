# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Netlink Wrapper Library"
DESCRIPTION = "library to provide Netlink wrapper functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://nl-wrapper.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://nl-wrapper.c \
    file://nl-wrapper.h \
    "

inherit meson pkgconfig

DEPENDS += "libncsi libnl"
