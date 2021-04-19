# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "FBGC Fruid Library"
DESCRIPTION = "library for fbgc fruid information"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fbgc_fruid.c;beginline=8;endline=20;md5=435eababecd3f367d90616c70e27bdd6"

inherit meson

SRC_URI = "file://fbgc_fruid \
          "

DEPENDS += "libfbgc-common libbic"
RDEPENDS_${PN} += "libfbgc-common libbic"

S = "${WORKDIR}/fbgc_fruid"

BBCLASSEXTEND = "native nativesdk"
