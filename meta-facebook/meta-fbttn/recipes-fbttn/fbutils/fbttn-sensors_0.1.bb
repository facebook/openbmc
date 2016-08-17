# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBTTN Sensor Utility"
DESCRIPTION = "Util for reading various sensors on FBTTN"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fbttn-sensors.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://fbttn-sensors \
          "

S = "${WORKDIR}/fbttn-sensors"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 fbttn-sensors ${D}${bindir}/fbttn-sensors
}

DEPENDS += "libfbttn-sensor"

FILES_${PN} = "${bindir}"
