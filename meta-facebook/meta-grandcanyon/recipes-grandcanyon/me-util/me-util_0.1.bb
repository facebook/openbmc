# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "Management Engine Utility"
DESCRIPTION = "Util for communicating to Intel ME"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://me-util.c;beginline=4;endline=16;md5=b66b777f082370423b0fa6f12a3dc4db"

inherit meson

SRC_URI = "file://me-util.c \
           file://meson.build \
          "

S = "${WORKDIR}"

DEPENDS = "libbic libpal libipmi libipmb libfbgc-common"
RDEPENDS_${PN} += "libbic libpal libipmi libipmb libfbgc-common"

