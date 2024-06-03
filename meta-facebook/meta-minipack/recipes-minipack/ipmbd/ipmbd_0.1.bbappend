# Copyright 2018-present Facebook. All Rights Reserved.

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_0.sh \
    file://run-ipmbd_4.sh \
    file://ipmbd_0.service \
    file://ipmbd_4.service \
    "
RDEPENDS:${PN} += " libbic jansson libipmb"

CFLAGS:prepend = " -DCONFIG_MINIPACK "

do_work_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_0
  install -d ${D}${sysconfdir}/sv/ipmbd_4
  install -d ${D}${sysconfdir}/ipmbd_0
  install -d ${D}${sysconfdir}/ipmbd_4
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_0.sh ${D}${sysconfdir}/sv/ipmbd_0/run
  install -m 755 ${S}/run-ipmbd_4.sh ${D}${sysconfdir}/sv/ipmbd_4/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

do_work_systemd() {
  install -d ${D}${systemd_system_unitdir}

  install -m 0644 ${S}/ipmbd_0.service ${D}${systemd_system_unitdir}
  install -m 0644 ${S}/ipmbd_4.service ${D}${systemd_system_unitdir}
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    do_work_systemd
  else
    do_work_sysv
  fi
}

SYSTEMD_SERVICE:${PN} += "ipmbd_0.service ipmbd_4.service"
