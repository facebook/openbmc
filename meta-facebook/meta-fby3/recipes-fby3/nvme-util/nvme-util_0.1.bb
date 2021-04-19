# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NVMe Utility"
DESCRIPTION = "A NVMe Management Util for Display I2C read write result"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://nvme-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

SRC_URI = "file://nvme-util.c \
           file://Makefile \
          "

CFLAGS += " -Wall -Werror -lbic -D_XOPEN_SOURCE "
LDFLAGS += " -lbic -lpal -lfby3_common"
DEPENDS += " libbic libpal libfby3-common "
RDEPENDS_${PN} += " libbic libpal libfby3-common "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 nvme-util ${D}${bindir}/nvme-util
}

FILES_${PN} = " ${bindir} "

