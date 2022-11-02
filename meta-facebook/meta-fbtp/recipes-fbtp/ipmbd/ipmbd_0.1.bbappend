# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_1.sh \
    file://run-ipmbd_4.sh \
    file://run-ipmbd_9.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"

CFLAGS:prepend = " -DCONFIG_FBTP"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_1
  install -d ${D}${sysconfdir}/sv/ipmbd_4
  install -d ${D}${sysconfdir}/sv/ipmbd_9
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_1.sh ${D}${sysconfdir}/sv/ipmbd_1/run
  install -m 755 ${S}/run-ipmbd_4.sh ${D}${sysconfdir}/sv/ipmbd_4/run
  install -m 755 ${S}/run-ipmbd_9.sh ${D}${sysconfdir}/sv/ipmbd_9/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

