# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Yosemite Sensor Utility"
DESCRIPTION = "Util for reading various sensors on Yosemite"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://yosemite-sensors.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://yosemite-sensors \
          "

S = "${WORKDIR}/yosemite-sensors"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 yosemite-sensors ${D}${bindir}/yosemite-sensors
}

DEPENDS += "libyosemite-sensor"

RDEPENDS:${PN} += "libyosemite-sensor"

FILES:${PN} = "${bindir}"
