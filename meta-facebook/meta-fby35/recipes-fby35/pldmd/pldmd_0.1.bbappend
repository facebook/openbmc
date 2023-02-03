# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-pldmd.sh \
    file://run-pldmd_1.sh \
    file://run-pldmd_2.sh \
    file://run-pldmd_3.sh \
    file://run-pldmd_4.sh \
    "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/pldmd_1
  install -d ${D}${sysconfdir}/sv/pldmd_2
  install -d ${D}${sysconfdir}/sv/pldmd_3
  install -d ${D}${sysconfdir}/sv/pldmd_4

  install -m 755 ${S}/setup-pldmd.sh ${D}${sysconfdir}/init.d/setup-pldmd.sh
  install -m 755 ${S}/run-pldmd_1.sh ${D}${sysconfdir}/sv/pldmd_1/run
  install -m 755 ${S}/run-pldmd_2.sh ${D}${sysconfdir}/sv/pldmd_2/run
  install -m 755 ${S}/run-pldmd_3.sh ${D}${sysconfdir}/sv/pldmd_3/run
  install -m 755 ${S}/run-pldmd_4.sh ${D}${sysconfdir}/sv/pldmd_4/run
  update-rc.d -r ${D} setup-pldmd.sh start 63 5 .
}
