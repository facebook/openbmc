# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Front Panel Control Utility"
DESCRIPTION = "Util to override the front panel control remotely"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://gpv2-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://gpv2-util \
          "
LDFLAGS = " -ljansson"
S = "${WORKDIR}/gpv2-util"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 gpv2-util ${D}${bindir}/gpv2-util
}

DEPENDS += "libpal jansson libfby2-sensor"
RDEPENDS:${PN} += "libbic libpal jansson libfby2-sensor"

FILES:${PN} = "${bindir}"
