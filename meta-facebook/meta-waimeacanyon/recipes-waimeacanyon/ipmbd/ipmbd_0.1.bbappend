# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_1.sh \
    file://run-ipmbd_2.sh \
    file://run-ipmbd_6.sh \
    file://run-ipmbd_7.sh \
    file://run-ipmbd_8.sh \
    file://run-ipmbd_9.sh \
    file://run-ipmbd_11.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_1
  install -d ${D}${sysconfdir}/sv/ipmbd_2
  install -d ${D}${sysconfdir}/sv/ipmbd_6
  install -d ${D}${sysconfdir}/sv/ipmbd_7
  install -d ${D}${sysconfdir}/sv/ipmbd_8
  install -d ${D}${sysconfdir}/sv/ipmbd_9
  install -d ${D}${sysconfdir}/sv/ipmbd_11
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_1.sh ${D}${sysconfdir}/sv/ipmbd_1/run
  install -m 755 ${S}/run-ipmbd_2.sh ${D}${sysconfdir}/sv/ipmbd_2/run
  install -m 755 ${S}/run-ipmbd_6.sh ${D}${sysconfdir}/sv/ipmbd_6/run
  install -m 755 ${S}/run-ipmbd_7.sh ${D}${sysconfdir}/sv/ipmbd_7/run
  install -m 755 ${S}/run-ipmbd_8.sh ${D}${sysconfdir}/sv/ipmbd_8/run
  install -m 755 ${S}/run-ipmbd_9.sh ${D}${sysconfdir}/sv/ipmbd_9/run
  install -m 755 ${S}/run-ipmbd_11.sh ${D}${sysconfdir}/sv/ipmbd_11/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

RDEPENDS:${PN}:append = " bash"
