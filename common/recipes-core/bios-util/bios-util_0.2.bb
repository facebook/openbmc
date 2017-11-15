# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "BIOS Utility"
DESCRIPTION = "A BIOS Util for BIOS Related IPMI Command Setting"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bios-util.py;beginline=1;endline=1;md5=d62248ab691cbaba10ec14c16cf8f625"

SRC_URI = "file://bios-util.py \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 bios-util.py ${D}${bindir}/bios-util
}

RDEPENDS_${PN} += "python3-core"

FILES_${PN} = "${prefix}/bin"
