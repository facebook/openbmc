# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Expander Utility"
DESCRIPTION = "Util for checking with Bridge IC on FBY35"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://expander-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://expander-util \
          "

S = "${WORKDIR}/expander-util"

inherit meson pkgconfig

DEPENDS += "libexp libpal"
RDEPENDS:${PN} += "libexp libpal"

FILES:${PN} = "${bindir}"
