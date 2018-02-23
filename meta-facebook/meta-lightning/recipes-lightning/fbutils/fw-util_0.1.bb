# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Firmware Utility"
DESCRIPTION = "Util for printing or updating firmware images"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fw-util.c;beginline=4;endline=16;md5=5f8ba3cd0f216026550dbcc0186d5599"

SRC_URI = "file://fw-util \
          "

S = "${WORKDIR}/fw-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 fw-util ${D}${bindir}/fw-util
}

DEPENDS += " libpal"
RDEPENDS_${PN} += "libpal"

FILES_${PN} = "${bindir}"
