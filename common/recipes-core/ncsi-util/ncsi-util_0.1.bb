# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NCSI Utility"
DESCRIPTION = "Util for sending NC-SI command to NIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ncsi-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build \
    file://ncsi-util.c \
    file://ncsi-util.h \
    file://brcm-ncsi-util.c \
    file://brcm-ncsi-util.h \
    file://nvidia-ncsi-util.c \
    file://nvidia-ncsi-util.h \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "ncsi-util"

DEPENDS += "libpal libncsi obmc-libpldm libnl-wrapper libkv zlib"

