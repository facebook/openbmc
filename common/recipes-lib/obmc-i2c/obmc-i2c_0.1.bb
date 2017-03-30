# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Common I2C device operations"
DESCRIPTION = "Headers to perform basic I2C operations"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://obmc-i2c.h;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://obmc-i2c.h \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}${includedir}/openbmc
    install -m 0644 obmc-i2c.h ${D}${includedir}/openbmc/obmc-i2c.h
}

