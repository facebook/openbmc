# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NAME Utility"
DESCRIPTION = "Util for showing valid FRU name on the platform"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://name-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit meson pkgconfig
inherit ptest-meson

LOCAL_URI = " \
    file://meson.build \
    file://name-util.cpp \
    file://name.hpp \
    file://tests/name-test.cpp \
    "

DEPENDS += "libpal cli11 nlohmann-json gtest"
RDEPENDS:${PN} += "libpal"
FILES:${PN} = "${prefix}/local/bin/name-util"
