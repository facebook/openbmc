# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Dimm library"
DESCRIPTION = "DIMM library"
SECTION = "base"
PR = "r1"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://dimm-base.cpp;beginline=4;endline=16;md5=417473877b7959f386857ca3ecd515a0"

LOCAL_URI = " \
    file://meson.build \
    file://plat/meson.build \
    file://dimm.h \
    file://dimm-vendor.cpp \
    file://dimm-capacity.cpp \
    file://dimm-pmic.cpp \
    file://dimm-base.cpp \
    "

inherit meson pkgconfig

DEPENDS += "libkv jansson libipmi"
FILES:${PN} = "${sysconfdir} ${prefix}/lib"
