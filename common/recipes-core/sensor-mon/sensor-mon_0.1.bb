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
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sensord.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit systemd

SRC_URI = "file://Makefile \
           file://sensord.c \
           file://sensord.service \
           file://setup-sensord.sh \
           file://run-sensord.sh \
          "

S = "${WORKDIR}"

SENSORD_MONITORED_FRUS ?= ""

binfiles = "sensord \
           "

CFLAGS += " -lsdr -lpal -laggregate-sensor "

DEPENDS += " libpal libsdr libaggregate-sensor update-rc.d-native"
RDEPENDS_${PN} += "libpal libsdr libaggregate-sensor bash "

pkgdir = "sensor-mon"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/sensord
  install -d ${D}${sysconfdir}/sensord
  sed -i 's/SENSORD_LAUNCH_ARGS/${SENSORD_MONITORED_FRUS}/g' run-sensord.sh
  install -m 755 setup-sensord.sh ${D}${sysconfdir}/init.d/setup-sensord.sh
  install -m 755 run-sensord.sh ${D}${sysconfdir}/sv/sensord/run
  update-rc.d -r ${D} setup-sensord.sh start 91 5 .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    sed -i 's/SENSORD_LAUNCH_ARGS/${SENSORD_MONITORED_FRUS}/g' sensord.service
    install -m 644 sensord.service ${D}${systemd_system_unitdir}
}

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/sensor-mon ${prefix}/local/bin ${sysconfdir} ${systemd_system_unitdir}"

SYSTEMD_SERVICE_${PN} = "sensord.service"
