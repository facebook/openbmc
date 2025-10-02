# Copyright 2024-present Meta Platforms Inc. All Rights Reserved.
SUMMARY = "Retimer Utility"
DESCRIPTION = "Utility for retimer"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${UNPACKDIR}/retimer-util.cpp;beginline=4;endline=16;md5=82c803198379a98ed2f8d54b5560f5e9"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://meson.build \
    file://retimer-util.cpp \
    "

inherit meson pkgconfig
DEPENDS += "cli11 libretimer libkv libmisc-utils"

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/retimer-util ${prefix}/local/bin"
