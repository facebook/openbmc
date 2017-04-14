# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing common sensor information access"
DESCRIPTION = "Library providing common sensor access"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://obmc-sensor.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://Makefile \
           file://obmc-sensor.c \
           file://obmc-sensor.h \
          "

S = "${WORKDIR}"

DEPENDS = " libpal libedb "

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libobmc-sensor.so ${D}${libdir}/libobmc-sensor.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 obmc-sensor.h ${D}${includedir}/openbmc/obmc-sensor.h
}

FILES_${PN} = "${libdir}/libobmc-sensor.so"
FILES_${PN}-dev = "${includedir}/openbmc/obmc-sensor.h"
