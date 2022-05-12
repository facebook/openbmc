# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "MCU Library"
DESCRIPTION = "MCU Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mcu.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://mcu.c \
    file://mcu.h \
    "

LDFLAGS += "-lipmb -lobmc-i2c -lpal -lkv"
DEPENDS = " libipmb libipmi libobmc-i2c libpal libkv"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libmcu.so ${D}${libdir}/libmcu.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 mcu.h ${D}${includedir}/openbmc/mcu.h
}

FILES:${PN} = "${libdir}/libmcu.so"
FILES:${PN}-dev = "${includedir}/openbmc/mcu.h"
RDEPENDS:${PN} = "libipmb libipmi libobmc-i2c libpal libkv"
