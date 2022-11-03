# Copyright 2016-present Facebook. All Rights Reserved.
SUMMARY = "Fan Utility"
DESCRIPTION = "Util for controlling the Fan Speed"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fan-util.cpp;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://fan-util.cpp \
    "

inherit meson pkgconfig
inherit legacy-packages

DEPENDS += "libpal cli11"

pkgdir = "fan-util"
