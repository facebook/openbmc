# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd-common.sh \
    file://setup-ipmbd-cover.sh \
    file://run-ipmbd_6.sh \
    file://run-ipmbd_14.sh \
    "

DEPENDS:append = " plat-utils update-rc.d-native"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_6
  install -d ${D}${sysconfdir}/sv/ipmbd_14
  install -m 755 ${S}/setup-ipmbd-common.sh ${D}${sysconfdir}/init.d/setup-ipmbd-common.sh
  install -m 755 ${S}/setup-ipmbd-cover.sh ${D}${sysconfdir}/init.d/setup-ipmbd-cover.sh
  install -m 755 ${S}/run-ipmbd_6.sh ${D}${sysconfdir}/sv/ipmbd_6/run
  install -m 755 ${S}/run-ipmbd_14.sh ${D}${sysconfdir}/sv/ipmbd_14/run
  update-rc.d -r ${D} setup-ipmbd-common.sh start 65 5 .
  update-rc.d -r ${D} setup-ipmbd-cover.sh start 66 5 .
}

