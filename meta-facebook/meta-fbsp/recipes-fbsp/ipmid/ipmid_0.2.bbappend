# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += " file://fruid.c \
          "
DEPENDS_append = " plat-utils update-rc.d-native"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "
