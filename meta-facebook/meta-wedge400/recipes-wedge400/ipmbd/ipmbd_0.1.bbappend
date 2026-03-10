#
# Copyright 2019-present Facebook. All Rights Reserved.
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
    file://run-ipmbd.sh \
    file://ipmbd@.service \
    file://ipmbd.target \
    "
RDEPENDS:${PN} += " libbic jansson libipmb bash"

CFLAGS:prepend = " -DCONFIG_WEDGE400 -DUSE_SLAVE_MQUEUE"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_0
  install -d ${D}${sysconfdir}/ipmbd_0
  install -d ${D}${sysconfdir}/sv/ipmbd_4
  install -d ${D}${sysconfdir}/ipmbd_4
  install -m 755 ${UNPACKDIR}/setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 ${UNPACKDIR}/run-ipmbd_0.sh ${D}${sysconfdir}/sv/ipmbd_0/run
  install -m 755 ${UNPACKDIR}/run-ipmbd_4.sh ${D}${sysconfdir}/sv/ipmbd_4/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -m 0644 ${UNPACKDIR}/ipmbd@.service ${D}${systemd_system_unitdir}
  install -m 0644 ${UNPACKDIR}/ipmbd.target ${D}${systemd_system_unitdir}
  install -d ${D}${prefix}/local/bin
  install -m 0755 ${UNPACKDIR}/run-ipmbd.sh ${D}${prefix}/local/bin/run-ipmbd.sh
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    install_systemd
  else
    install_sysv
  fi
}

SYSTEMD_SERVICE:${PN} += "ipmbd@.service ipmbd.target"

