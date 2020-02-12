# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Voltage Regulator Library"
DESCRIPTION = "Library for communication with the voltage regulator"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://vr.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://vr.c \
           file://vr.h \
           file://mpq8645p.c \
           file://mpq8645p.h \
           file://pxe1110c.c \
           file://pxe1110c.h \
           file://tps53688.c \
           file://tps53688.h \
           file://xdpe12284c.c \
           file://xdpe12284c.h \
           file://platform.c \
           file://Makefile \
          "

LDFLAGS += "-lobmc-pmbus -lkv -lpal"
DEPENDS += "libobmc-pmbus libkv libpal"
RDEPENDS_${PN} += "libobmc-pmbus libkv libpal"

S = "${WORKDIR}"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libvr.so ${D}${libdir}/libvr.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 vr.h ${D}${includedir}/openbmc/vr.h
}

FILES_${PN} = "${libdir}/libvr.so"
FILES_${PN}-dev = "${includedir}/openbmc/vr.h ${includedir}/openbmc/pmbus.h"
