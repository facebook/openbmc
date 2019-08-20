# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "MCU Library"
DESCRIPTION = "MCU Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mcu.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://mcu.c \
           file://mcu.h \
          "

S = "${WORKDIR}"

LDFLAGS += "-lipmb -lobmc-i2c -lpal"
DEPENDS = " libipmb libipmi libobmc-i2c libpal"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libmcu.so ${D}${libdir}/libmcu.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 mcu.h ${D}${includedir}/openbmc/mcu.h
}

FILES_${PN} = "${libdir}/libmcu.so"
FILES_${PN}-dev = "${includedir}/openbmc/mcu.h"
RDEPENDS_${PN} = "libipmb libipmi libobmc-i2c libpal"
