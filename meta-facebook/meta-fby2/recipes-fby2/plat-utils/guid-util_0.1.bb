# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "GUID Utility"
DESCRIPTION = "Util for reading or updating GUID"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://guid-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://guid-util \
          "

S = "${WORKDIR}/guid-util"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 guid-util ${D}${bindir}/guid-util
}

DEPENDS += " libpal "
RDEPENDS:${PN} += "libpal"

FILES:${PN} = "${bindir}"
