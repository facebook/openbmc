# Copyright 2019-present Facebook. All Rights Reserved.
SUMMARY = "RMA Snapshot Utility"
DESCRIPTION = "Util for creating RMA snapshot on FBY2"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://snapshot-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://snapshot-util \
          "

S = "${WORKDIR}/snapshot-util"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 snapshot-util ${D}${bindir}/snapshot-util
}

DEPENDS += " libbic libpal "
RDEPENDS_${PN} += " libbic libpal "
FILES_${PN} = "${bindir}"
