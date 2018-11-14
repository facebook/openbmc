# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Bridge IC Utility"
DESCRIPTION = "Util for checking with Bridge IC on FBY2"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://bic-util \
          "

S = "${WORKDIR}/bic-util"

CFLAGS += " -Wall -Werror -lbic -lfby2_gpio -D_XOPEN_SOURCE"


do_install() {
	  install -d ${D}${bindir}
    install -m 0755 bic-util ${D}${bindir}/bic-util
}
DEPENDS += "libbic libpal libfby2-gpio libfby2-sensor"
RDEPENDS_${PN} += "libbic libpal libfby2-gpio libfby2-sensor "


FILES_${PN} = "${bindir}"
