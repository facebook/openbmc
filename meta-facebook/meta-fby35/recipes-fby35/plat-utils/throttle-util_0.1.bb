# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Throttle Utility"
DESCRIPTION = "Util for triggering with Throttle on FBY35"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://throttle-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://throttle-util"
S = "${WORKDIR}/throttle-util"

inherit meson pkgconfig

DEPENDS += "libobmc-i2c libpal libfby35-common"
