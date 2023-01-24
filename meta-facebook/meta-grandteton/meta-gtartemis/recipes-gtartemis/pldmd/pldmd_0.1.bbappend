# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://run-pldmd_9.sh \
    "

do_install:append() {
  install -d ${D}${sysconfdir}/sv/pldmd_9
  install -m 755 ${S}/run-pldmd_9.sh ${D}${sysconfdir}/sv/pldmd_9/run
}
