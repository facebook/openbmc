# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Yosemite Sensor Library"
DESCRIPTION = "library for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fbttn_sensor.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://fbttn_sensor \
          "
DEPENDS =+ " libipmi libipmb libbic libmctp libfbttn-common libobmc-i2c obmc-pal libnvme-mi "
RDEPENDS:${PN} += "libobmc-i2c libnvme-mi "
LDFLAGS += "-lobmc-i2c"

S = "${WORKDIR}/fbttn_sensor"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libfbttn_sensor.so ${D}${libdir}/libfbttn_sensor.so

    install -d ${D}${includedir}/facebook
    install -m 0644 fbttn_sensor.h ${D}${includedir}/facebook/fbttn_sensor.h
}

FILES:${PN} = "${libdir}/libfbttn_sensor.so"
FILES:${PN}-dev = "${includedir}/facebook/fbttn_sensor.h"
