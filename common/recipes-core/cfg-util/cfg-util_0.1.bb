# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Configuration Utility"
DESCRIPTION = "Utility for reading or writing configuration"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://cfg-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://meson.build \
    file://cfg-util.cpp \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "cfg-util"

DEPENDS += "libpal libkv libmisc-utils"

