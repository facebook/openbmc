# Copyright 2021-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += "file://syseventgen.conf"

do_install() {
  install -d ${D}${prefix}/local/bin
  install -d ${D}${sysconfdir}
  install -m 0755 ${WORKDIR}/build/syseventgen-util ${D}${prefix}/local/bin
  install -m 0644 ${S}/syseventgen.conf ${D}${sysconfdir}
}

FILES:${PN} = "${prefix}/local/bin ${sysconfdir} "
