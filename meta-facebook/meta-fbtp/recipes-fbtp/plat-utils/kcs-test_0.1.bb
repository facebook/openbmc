# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "KCS Test Utility"
DESCRIPTION = "Test for KCS interface"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://kcs-test.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://kcs-test \
          "

S = "${WORKDIR}/kcs-test"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 kcs-test ${D}${bindir}/kcs-test
}

DEPENDS += " "

FILES:${PN} = "${bindir}"
