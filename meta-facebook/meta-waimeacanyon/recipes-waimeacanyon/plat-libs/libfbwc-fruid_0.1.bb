# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBWC Fruid Library"
DESCRIPTION = "library for fbwc fruid information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fbwc_fruid.c;beginline=8;endline=19;md5=4aff7c1eb9efac80fc5d2f8e2ed82562"

inherit meson pkgconfig

SRC_URI = "file://fbwc_fruid \
          "

DEPENDS += "libfbwc-common libfruid libbic libexp"

S = "${WORKDIR}/fbwc_fruid"

BBCLASSEXTEND = "native nativesdk"
