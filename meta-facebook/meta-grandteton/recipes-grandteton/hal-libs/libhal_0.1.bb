# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "HAL FRUID Library"
DESCRIPTION = "HAL FRUID Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://hal_fruid.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://hal_fruid.c \
    file://hal_fruid.h \
    "

DEPENDS += "libipmi libpldm-oem"
LDFLAGS += "-lpldm_oem"


inherit meson pkgconfig
