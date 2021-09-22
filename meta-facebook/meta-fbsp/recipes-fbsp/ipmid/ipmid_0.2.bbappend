# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " file://fruid.c \
             file://usb-dbg-conf.c \
          "
DEPENDS += "libipmi libipmb libfruid update-rc.d-native"
RDEPENDS:${PN} += "libipmi libfruid libipmb "
LDFLAGS += "-lfruid -lipmb"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "
