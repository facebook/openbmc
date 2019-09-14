# Copyright 2019-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Utility"
DESCRIPTION = "Util for checking with Bridge IC on Wedge400"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-util.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"

SRC_URI = "file://bic-util \
          "

S = "${WORKDIR}/bic-util"

CFLAGS += " -llog -lbic -lipmi -lipmb -lwedge400_gpio -lfruid"


do_install() {
    install -d ${D}${bindir}
    install -m 0755 bic-util ${D}${bindir}/bic-util
}

DEPENDS += "liblog libbic libipmi libipmb libwedge400-gpio libfruid"
RDEPENDS_${PN} += "liblog libbic libipmi libipmb libwedge400-gpio libfruid"

FILES_${PN} = "${bindir}"
