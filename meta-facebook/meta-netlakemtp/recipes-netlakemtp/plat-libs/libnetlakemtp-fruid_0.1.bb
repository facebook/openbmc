# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "NETLAKE Fruid Library"
DESCRIPTION = "library for netlakemtp fruid information"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://netlakemtp_fruid.c;beginline=8;endline=20;md5=435eababecd3f367d90616c70e27bdd6"

inherit meson

SRC_URI = "file://netlakemtp_fruid \
          "

DEPENDS += "libnetlakemtp-common"
RDEPENDS_${PN} += "libnetlakemtp-common"

S = "${WORKDIR}/netlakemtp_fruid"

BBCLASSEXTEND = "native nativesdk"
