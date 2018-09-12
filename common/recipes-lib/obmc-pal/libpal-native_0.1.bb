# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

inherit native

SRC_URI = "file://obmc-pal.h \
           file://obmc-pal.c \
           file://pal.h \
           file://obmc-sensor.c \
           file://obmc-sensor.h \
           file://Makefile \
          "
DEPENDS += " libkv-native libipmi-native"

S = "${WORKDIR}"

do_compile() {
  make
}

do_install_append() {
  install -d ${D}${libdir}
  install -m 0644 libpal.so ${D}${libdir}/libpal.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 ${S}/obmc-pal.h ${D}${includedir}/openbmc/obmc-pal.h
  install -m 0644 ${S}/obmc-sensor.h ${D}${includedir}/openbmc/obmc-sensor.h
  install -m 0644 ${S}/pal.h ${D}${includedir}/openbmc/pal.h
}

RDEPENDS_${PN} += " libkv-native "
