# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "DIMM Utility"
DESCRIPTION = "Util to retrieve DIMM information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://dimm-util.cpp;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

LOCAL_URI = " \
    file://meson.build \
    file://dimm-util.cpp \
    file://dimm-util.h \
    file://dimm-vendor.cpp \
    file://dimm-capacity.cpp \
    file://dimm-pmic.cpp \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "dimm-util"

DEPENDS += "libpal libkv jansson"
