# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Power Utility"
DESCRIPTION = "Utility for Power Policy and Management"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://power-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build \
    file://power-util.c \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "power-util"

DEPENDS += "libpal"
