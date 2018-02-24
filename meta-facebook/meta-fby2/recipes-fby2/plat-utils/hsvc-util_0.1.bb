# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "HotService Utility"
DESCRIPTION = "Util to activate hot service for slot"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://hsvc-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://hsvc-util \
          "

S = "${WORKDIR}/hsvc-util"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 hsvc-util ${D}${bindir}/hsvc-util
}

DEPENDS += "libfby2-common libfby2-sensor libpal"
RDEPENDS_${PN} += "libfby2-common libfby2-sensor libpal"

FILES_${PN} = "${bindir}"
