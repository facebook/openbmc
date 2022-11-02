# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ipmbd tx/rx daemon"
DESCRIPTION = "The ipmb daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ipmbd.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://ipmbd.c \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "ipmbd"

DEPENDS += " \
    libipc \
    libipmb \
    libipmi \
    liblog \
    libmisc-utils \
    libobmc-i2c \
    libpal \
    update-rc.d-native \
"
