# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "GUID Utility"
DESCRIPTION = "Util for reading or updating GUID"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://guid-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://guid-util.c \
    file://meson.build \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "guid-util"

DEPENDS = "libpal"
