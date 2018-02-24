# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ipmbd tx/rx daemon"
DESCRIPTION = "The ipmb daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmbd.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://ipmbd.c \
          "

S = "${WORKDIR}"
DEPENDS += "libipmi libipmb obmc-i2c libpal"
DEPENDS += "update-rc.d-native"
RDEPENDS_${PN} = "libipmi libpal"

binfiles = "ipmbd"

pkgdir = "ipmbd"
