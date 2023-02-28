# Copyright 2022-present Meta Platforms Inc. All Rights Reserved.

SUMMARY = "HGX Library"
DESCRIPTION = "Daemon to monitor and control the accelerator board"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://hgx.cpp \
    file://hgx.h \
    file://time_utils.hpp \
    "

DEPENDS += "restclient-cpp nlohmann-json libkv"
RDEPENDS:${PN} += "restclient-cpp"
