# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "At Scale Debug Test util"
DESCRIPTION = "Test JTAG interface"
SECTION = "base"
PR = "r1"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://jtagtest.c;beginline=4;endline=25;md5=4d3dd6a70786d475883b1542b0898219"

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
