# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Firmware Utility"
DESCRIPTION = "Util for printing or updating firmware images"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fw-util.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"

SRC_URI = "file://fw-util \
          "

S = "${WORKDIR}/fw-util"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 fw-util ${D}${bindir}/fw-util
}

DEPENDS += "libbic libpal libipmb libipmi"
RDEPENDS_${PN} += "libbic libpal libipmb libipmi"

FILES_${PN} = "${bindir}"
