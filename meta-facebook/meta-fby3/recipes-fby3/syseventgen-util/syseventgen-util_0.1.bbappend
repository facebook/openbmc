# Copyright 2021-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://syseventgen.conf \
            "

do_install() {
  install -d ${D}${prefix}/local/bin
  install -d ${D}${sysconfdir}
  install -m 0755 ${WORKDIR}/build/syseventgen-util ${D}${prefix}/local/bin
  install -m 0644 ${WORKDIR}/syseventgen.conf ${D}${sysconfdir}
}

FILES_${PN} = "${prefix}/local/bin ${sysconfdir} "
