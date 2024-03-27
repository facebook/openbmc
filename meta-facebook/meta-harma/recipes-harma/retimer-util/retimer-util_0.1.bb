# Copyright 2022-present Meta Platforms Inc. All Rights Reserved.

SUMMARY = "Retimer Utility"
DESCRIPTION = "Utility for retimer"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://retimer-util.cpp \
    "

DEPENDS += "libretimer libpal cli11"

FILES:${PN} = "${prefix}/local/bin"
