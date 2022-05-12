# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "NVMe Utility"
DESCRIPTION = "A NVMe Management Util for Display I2C read write result"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://nvme-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

SRC_URI = "file://nvme-util \
          "

S = "${WORKDIR}/nvme-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 nvme-util ${D}${bindir}/nvme-util
}

LDFLAGS += "-lobmc-i2c"
DEPENDS += " libbic libfby2-sensor libpal libobmc-i2c"
RDEPENDS:${PN} += " libbic libfby2-sensor libpal libobmc-i2c"
FILES:${PN} = "${bindir}"
