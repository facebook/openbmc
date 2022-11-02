# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_3.sh \
    file://run-ipmbd_9.sh \
    file://run-ipmbd_11.sh \
    "
DEPENDS:append = " fbutils "

CFLAGS:prepend = " -DCONFIG_FBTTN"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_3
  install -d ${D}${sysconfdir}/sv/ipmbd_9
  install -d ${D}${sysconfdir}/sv/ipmbd_11
  install -d ${D}${sysconfdir}/ipmbd_3
  install -d ${D}${sysconfdir}/ipmbd_9
  install -d ${D}${sysconfdir}/ipmbd_11
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_3.sh ${D}${sysconfdir}/sv/ipmbd_3/run
  install -m 755 ${S}/run-ipmbd_9.sh ${D}${sysconfdir}/sv/ipmbd_9/run
  install -m 755 ${S}/run-ipmbd_11.sh ${D}${sysconfdir}/sv/ipmbd_11/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

