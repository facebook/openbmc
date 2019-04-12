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

LDFLAGS += "-lobmc-i2c -llog"

S = "${WORKDIR}"
DEPENDS += "libipmi libipmb libobmc-i2c libpal libipc liblog"
DEPENDS += "update-rc.d-native"
RDEPENDS_${PN} = "libipmi libpal libipc libobmc-i2c liblog"

binfiles = "ipmbd"

pkgdir = "ipmbd"
