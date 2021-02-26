# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

SRC_URI = "file://pal.c \
           file://pal.h \
           file://pal_sensors.h \
           file://Makefile \
           file://pal.py \
          "
DEPENDS += " libkv libipmi libipmb obmc-pal"
LDFLAGS += " -lkv -lipmi -lipmb"
SOURCES = "pal.c"
HEADERS = "pal.h pal_sensors.h"

CFLAGS += "-Wall -Werror -fPIC"

S = "${WORKDIR}"

inherit python3-dir

do_compile() {
  make SOURCES="${SOURCES}" HEADERS="${HEADERS}"
}

do_install_append() {
  install -d ${D}${libdir}
  install -m 0644 libpal.so ${D}${libdir}/libpal.so
  ln -s libpal.so ${D}${libdir}/libpal.so.0

  install -d ${D}${includedir}/openbmc
  for f in ${HEADERS}; do
    install -m 0644 ${S}/$f ${D}${includedir}/openbmc/$f
  done

  install -d ${D}${PYTHON_SITEPACKAGES_DIR}
  install -m 644 ${S}/pal.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}

RDEPENDS_${PN} += " libkv "

FILES_${PN} = "${libdir}/libpal.so* ${PYTHON_SITEPACKAGES_DIR}/pal.py"
FILES_${PN}-dev = "${includedir}/openbmc"
