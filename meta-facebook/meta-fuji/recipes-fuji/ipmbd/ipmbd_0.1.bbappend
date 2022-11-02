#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_0.sh \
    file://run-ipmbd_4.sh \
    file://ipmbd.service \
    "

RDEPENDS:${PN} += "bash"


do_work_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_0
  install -d ${D}${sysconfdir}/ipmbd_0
  install -d ${D}${sysconfdir}/sv/ipmbd_4
  install -d ${D}${sysconfdir}/ipmbd_4
  install -m 755 ${S}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_0.sh ${D}${sysconfdir}/sv/ipmbd_0/run
  install -m 755 ${S}/run-ipmbd_4.sh ${D}${sysconfdir}/sv/ipmbd_4/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

do_work_systemd() {
  install -d ${D}/usr/local/bin
  install -d ${D}${systemd_system_unitdir}

  install -m 755 ${S}/setup-ipmbd.sh ${D}/usr/local/bin/setup-ipmbd.sh
  install -m 755 ${S}/run-ipmbd_0.sh ${D}/usr/local/bin/ipmbd_0
  install -m 755 ${S}/run-ipmbd_4.sh ${D}/usr/local/bin/ipmbd_4

  install -m 0644 ${S}/ipmbd.service ${D}${systemd_system_unitdir}

  sed -i -e 's/runsv \"\/etc\/sv/\"\/usr\/local\/bin/' ${D}/usr/local/bin/setup-ipmbd.sh
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    do_work_systemd
  else
    do_work_sysv
  fi
}

SYSTEMD_SERVICE:${PN} += "ipmbd.service"
