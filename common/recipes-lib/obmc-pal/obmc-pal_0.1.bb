# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

SRC_URI = "file://obmc-pal.h \
           file://obmc_pal_sensors.h \
          "
S = "${WORKDIR}"
# Dummy compile step. We just install the headers in this recipe.
do_compile() {
  true
}
#S = "${WORKDIR}"
do_install() {
  install -d ${D}${includedir}/openbmc
  cp obmc-pal.h ${D}${includedir}/openbmc/obmc-pal.h
  cp obmc_pal_sensors.h ${D}${includedir}/openbmc/obmc_pal_sensors.h
}

FILES_${PN}-dev += "${includedir}openbmc/obmc-pal.h ${includedir}openbmc/obmc_pal_sensors.h"
DEPENDS += " libkv libipmi libipmb"
