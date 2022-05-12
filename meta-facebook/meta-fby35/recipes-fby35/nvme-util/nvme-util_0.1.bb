# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NVMe Utility"
DESCRIPTION = "A NVMe Management Util for Display I2C read write result"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://nvme-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

LOCAL_URI = " \
    file://nvme-util.c \
    file://Makefile \
    "

CFLAGS += " -Wall -Werror -lbic -D_XOPEN_SOURCE "
LDFLAGS += " -lbic -lpal -lfby35_common"
DEPENDS += " libbic libpal libfby35-common "
RDEPENDS:${PN} += " libbic libpal libfby35-common "

do_install() {
  install -d ${D}${bindir}
  install -m 0755 nvme-util ${D}${bindir}/nvme-util
}

FILES:${PN} = " ${bindir} "

