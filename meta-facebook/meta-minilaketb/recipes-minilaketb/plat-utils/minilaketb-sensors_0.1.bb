# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "minilaketb Sensor Utility"
DESCRIPTION = "Util for reading various sensors on minilaketb"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://minilaketb-sensors.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://minilaketb-sensors \
          "

S = "${WORKDIR}/minilaketb-sensors"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 minilaketb-sensors ${D}${bindir}/minilaketb-sensors
}

DEPENDS += "libminilaketb-sensor"
RDEPENDS:${PN} += "libminilaketb-sensor"

FILES:${PN} = "${bindir}"
