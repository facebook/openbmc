# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Throttle Utility"
DESCRIPTION = "Util for triggering with Throttle on FBY3"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://throttle-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://throttle-util \
          "

S = "${WORKDIR}/throttle-util"

CFLAGS += "-Wall -Werror -lbic -D_XOPEN_SOURCE"
LDFLAGS = " -lobmc-i2c -lbic -lpal -lfby3_common -lfby3_gpio "
DEPENDS += " libbic libpal libfby3-common libfby3-gpio libobmc-i2c "
RDEPENDS_${PN} += " libbic libpal libfby3-common libfby3-gpio libobmc-i2c "

do_install() {
  install -d ${D}${bindir}
  install -m 0755 throttle-util ${D}${bindir}/throttle-util
}

FILES_${PN} = "${bindir}"
