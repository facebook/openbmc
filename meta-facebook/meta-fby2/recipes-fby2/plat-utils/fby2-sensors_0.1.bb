# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "fby2 Sensor Utility"
DESCRIPTION = "Util for reading various sensors on fby2"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fby2-sensors.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://fby2-sensors \
          "

S = "${WORKDIR}/fby2-sensors"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 fby2-sensors ${D}${bindir}/fby2-sensors
}

DEPENDS += "libfby2-common libfby2-sensor"
RDEPENDS_${PN} += "libfby2-common libfby2-sensor"

FILES_${PN} = "${bindir}"
