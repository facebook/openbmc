# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "pldm daemon"
DESCRIPTION = "The pldm daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pldmd.cpp;beginline=4;endline=16;md5=d58e137b86f021046174d6ff57a454bb"

inherit meson pkgconfig

RDEPENDS:${PN} = " libpldm libpldm-oem"
DEPENDS = " libpldm libpldm-oem cli11 update-rc.d-native"


LOCAL_URI = " \
    file://meson.build \
    file://pldmd.cpp \
    file://instance_id.hpp \
    file://instance_id.cpp \
    "
pkgdir = "pldmd"

FILES:${PN} = "${prefix}/local/bin/pldmd ${sysconfdir}"
