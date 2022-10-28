# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Threshold Utility"
DESCRIPTION = "Util for modify sensor threshold temporarily"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://threshold-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build \
    file://threshold-util.c \
    "

inherit meson pkgconfig
inherit legacy-packages

DEPENDS =+ " libsdr libpal "
RDEPENDS:${PN} =+ "libsdr libpal "

pkgdir = "threshold-util"
