# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "IPMB Utility"
DESCRIPTION = "Util for communicating with an IPMB endpoint"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ipmb-util.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://ipmb-util.c \
    file://meson.build\
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "ipmb-util"

DEPENDS += "libipmb libipmi"
