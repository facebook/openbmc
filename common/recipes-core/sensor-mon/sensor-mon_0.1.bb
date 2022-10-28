# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "Sensor Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sensord.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit systemd

LOCAL_URI = " \
    file://meson.build \
    file://sensord.c \
    file://sensord.service \
    file://setup-sensord.sh \
    file://run-sensord.sh \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "sensor-mon"

SENSORD_MONITORED_FRUS ?= ""

DEPENDS += " libpal libsdr libaggregate-sensor update-rc.d-native"
RDEPENDS:${PN} += "bash"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/sensord
  install -d ${D}${sysconfdir}/sensord
  sed -i 's/SENSORD_LAUNCH_ARGS/${SENSORD_MONITORED_FRUS}/g' ${S}/run-sensord.sh
  install -m 755 ${S}/setup-sensord.sh ${D}${sysconfdir}/init.d/setup-sensord.sh
  install -m 755 ${S}/run-sensord.sh ${D}${sysconfdir}/sv/sensord/run
  update-rc.d -r ${D} setup-sensord.sh start 91 5 .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    sed -i 's/SENSORD_LAUNCH_ARGS/${SENSORD_MONITORED_FRUS}/g' ${S}/sensord.service
    install -m 644 ${S}/sensord.service ${D}${systemd_system_unitdir}
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi
}

#FILES:${PN} = "${FBPACKAGEDIR}/sensor-mon ${prefix}/local/bin ${sysconfdir} ${systemd_system_unitdir}"

SYSTEMD_SERVICE:${PN} = "sensord.service"
