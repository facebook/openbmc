# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "MINILAKETB Sensor Library"
DESCRIPTION = "library for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://minilaketb_sensor.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://minilaketb_sensor \
          "
LDFLAGS += "-lobmc-i2c"
DEPENDS =+ " libipmi libipmb libbic libminilaketb-common plat-utils libobmc-i2c libnvme-mi obmc-pal "

S = "${WORKDIR}/minilaketb_sensor"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libminilaketb_sensor.so ${D}${libdir}/libminilaketb_sensor.so

    install -d ${D}${includedir}/facebook
    install -m 0644 minilaketb_sensor.h ${D}${includedir}/facebook/minilaketb_sensor.h
}

FILES:${PN} = "${libdir}/libminilaketb_sensor.so"
FILES:${PN}-dev = "${includedir}/facebook/minilaketb_sensor.h"

RDEPENDS:${PN} += " libnvme-mi minilaketb-sensors libobmc-i2c"
