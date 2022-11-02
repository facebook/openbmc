# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_1.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"
CFLAGS:prepend = "-DTIMEOUT_IPMB=3"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_1
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_1.sh ${D}${sysconfdir}/sv/ipmbd_1/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

