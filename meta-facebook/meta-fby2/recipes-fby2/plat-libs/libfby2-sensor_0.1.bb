# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "FBY2 Sensor Library"
DESCRIPTION = "library for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fby2_sensor.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fby2_sensor \
          "
LDFLAGS += "-lobmc-i2c"
DEPENDS =+ " libipmi libipmb libbic libfby2-common plat-utils libobmc-i2c libobmc-sensors libnvme-mi obmc-pal "

S = "${WORKDIR}/fby2_sensor"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libfby2_sensor.so ${D}${libdir}/libfby2_sensor.so

    install -d ${D}${includedir}/facebook
    install -m 0644 fby2_sensor.h ${D}${includedir}/facebook/fby2_sensor.h
}

FILES:${PN} = "${libdir}/libfby2_sensor.so"
FILES:${PN}-dev = "${includedir}/facebook/fby2_sensor.h"

RDEPENDS:${PN} += " libnvme-mi fby2-sensors libobmc-i2c libobmc-sensors libbic libfby2-common"
