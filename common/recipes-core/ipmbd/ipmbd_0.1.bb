# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ipmbd tx/rx daemon"
DESCRIPTION = "The ipmb daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ipmbd.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://ipmbd.c \
    "

LDFLAGS += "-lobmc-i2c -llog -lmisc-utils"

DEPENDS += "libipmi libipmb libobmc-i2c libpal libipc liblog libmisc-utils"
DEPENDS += "update-rc.d-native"
RDEPENDS:${PN} = "libipmi libpal libipc libobmc-i2c liblog libmisc-utils"

binfiles = "ipmbd"

pkgdir = "ipmbd"
