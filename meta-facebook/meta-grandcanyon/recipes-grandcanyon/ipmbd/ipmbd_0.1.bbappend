# Copyright 2020-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_2.sh \
    file://run-ipmbd_7.sh \
    file://run-ipmbd_10.sh \
    "
#DEPENDS:append = " plat-utils "
CFLAGS:prepend = " -DTIMEOUT_IPMB=3 "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_2
  install -d ${D}${sysconfdir}/sv/ipmbd_7
  install -d ${D}${sysconfdir}/sv/ipmbd_10
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_2.sh ${D}${sysconfdir}/sv/ipmbd_2/run
  install -m 755 ${S}/run-ipmbd_7.sh ${D}${sysconfdir}/sv/ipmbd_7/run
  install -m 755 ${S}/run-ipmbd_10.sh ${D}${sysconfdir}/sv/ipmbd_10/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

