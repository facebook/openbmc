# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Utility"
DESCRIPTION = "Util for checking with Bridge IC on FBY35"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://bic-util"
S = "${WORKDIR}/bic-util"

inherit meson pkgconfig

DEPENDS += "libbic libpal libfby35-common libfby35-gpio"
