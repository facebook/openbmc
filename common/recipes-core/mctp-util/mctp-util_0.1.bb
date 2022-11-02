# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "MCTP Utility"
DESCRIPTION = "Util for sending MCTP command to a device"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mctp-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build\
    file://decode.cpp \
    file://decode.h \
    file://mctp-util.cpp \
    file://mctp-util.h \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "mctp-util"

DEPENDS += "libpal libobmc-mctp"
