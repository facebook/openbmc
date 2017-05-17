# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "At Scale Debug Test util"
DESCRIPTION = "Test JTAG interface"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://jtagtest.c;beginline=6;endline=13;md5=678b5e5a5bd177043fb4177e772c804d"

SRC_URI = "file://test \
          "
DEPENDS += " libasd-jtagintf "

S = "${WORKDIR}/test"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 asd-test ${D}${bindir}/asd-test
}

FILES_${PN} = "${bindir}"
RDEPENDS_${PN} = "libasd-jtagintf"
