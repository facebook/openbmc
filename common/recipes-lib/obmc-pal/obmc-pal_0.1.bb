# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPL-2.1-only"

# The license LGPL-2.1 was removed in Hardknott.
# Use LGPL-2.1-only instead.
def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

    return "LGPL-2.1-only;md5=1a6d268fd218675ffea8be556788b780"

LIC_FILES_CHKSUM = "\
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \
    "

LOCAL_URI = " \
    file://obmc-pal.h \
    file://obmc_pal_sensors.h \
    "

# Dummy compile step. We just install the headers in this recipe.
do_compile() {
  true
}
do_install() {
  install -d ${D}${includedir}/openbmc
  cp obmc-pal.h ${D}${includedir}/openbmc/obmc-pal.h
  cp obmc_pal_sensors.h ${D}${includedir}/openbmc/obmc_pal_sensors.h
}

FILES:${PN}-dev += "${includedir}openbmc/obmc-pal.h ${includedir}openbmc/obmc_pal_sensors.h"
DEPENDS += " libkv libipmi libipmb"
