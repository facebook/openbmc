# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "IPMI Utility"
DESCRIPTION = "Util for interacting with IPMI stack"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ipmi-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://ipmi-util.c \
    file://meson.build\
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "ipmi-util"

DEPENDS += "libipmi libpal"

