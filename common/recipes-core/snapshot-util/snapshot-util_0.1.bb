# Copyright 2019-present Facebook. All Rights Reserved.
SUMMARY = "RMA Snapshot Utility"
DESCRIPTION = "Util for creating RMA snapshot"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://snapshot-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build \
    file://snapshot-util.c \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "snapshot-util"

DEPENDS += "libbic libpal"
RDEPENDS:${PN} += "libbic"
