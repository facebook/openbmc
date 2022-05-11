# Copyright 2021-present Facebook. All Rights Reserved.
SUMMARY = "Throttle Utility"
DESCRIPTION = "Util for triggering with Throttle"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://throttle-util.c;beginline=4;endline=16;md5=b66b777f082370423b0fa6f12a3dc4db"

inherit meson pkgconfig

SRC_URI = "file://throttle-util \
          "
S = "${WORKDIR}/throttle-util"

DEPENDS += " libbic libpal libfbgc-common libobmc-i2c "
RDEPENDS:${PN} += " libbic libpal libfbgc-common libobmc-i2c "

FILES:${PN} = "${bindir}"
