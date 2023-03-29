# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Utility"
DESCRIPTION = "Util for checking with Bridge IC on FBY3"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://bic-util \
          "

S = "${WORKDIR}/bic-util"

CFLAGS += "-Wall -Werror -lbic -D_XOPEN_SOURCE"
LDFLAGS = "-lbic -lpal -lfby3_common -lfby3_gpio -lusb-1.0"
DEPENDS += "libbic libpal libfby3-common libfby3-gpio"
RDEPENDS:${PN} += "libbic libfby3-common libfby3-gpio"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 bic-util ${D}${bindir}/bic-util
}

FILES:${PN} = "${bindir}"
