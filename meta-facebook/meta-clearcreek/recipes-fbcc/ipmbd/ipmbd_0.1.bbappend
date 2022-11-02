# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_8.sh \
    file://run-ipmbd_0.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_8
  install -d ${D}${sysconfdir}/sv/ipmbd_0
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_8.sh ${D}${sysconfdir}/sv/ipmbd_8/run
  install -m 755 ${S}/run-ipmbd_0.sh ${D}${sysconfdir}/sv/ipmbd_0/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}
