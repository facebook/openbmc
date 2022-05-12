# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "Expander Library"
DESCRIPTION = "library for communicating with Expander"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://exp.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

SRC_URI = "file://exp \
          "
DEPENDS += " libipmi libipmb libfbgc-common "

S = "${WORKDIR}/exp"

BBCLASSEXTEND = "native nativesdk"
