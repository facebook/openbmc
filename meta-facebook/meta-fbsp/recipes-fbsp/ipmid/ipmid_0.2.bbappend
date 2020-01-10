# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += " file://fruid.c \
             file://usb-dbg-conf.c \
          "
DEPENDS += "libipmi libipmb libfruid update-rc.d-native"
RDEPENDS_${PN} += "libipmi libfruid libipmb "
LDFLAGS += "-lfruid -lipmb"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "
