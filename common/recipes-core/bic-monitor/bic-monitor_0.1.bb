# Copyright 2018-present Facebook. All Rights Reserved.
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

inherit systemd

SUMMARY = "GPIO Sensor Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the bic sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-monitor.c;beginline=4;endline=16;md5=5d75ad6348d98a3f7ee7e2be8db29e00"

LOCAL_URI = " \
    file://Makefile \
    file://bic-monitor.c \
    file://setup-bic-monitor.sh \
    file://run-bic-monitor.sh \
    file://bicmond.service \
    "

binfiles = "bicmond \
           "

CFLAGS += " -Werror "
LDFLAGS += " -lbic -lpal -llog -lmisc-utils"

DEPENDS += " libbic libpal liblog libmisc-utils update-rc.d-native"
RDEPENDS:${PN} += " libbic libpal liblog libmisc-utils bash"

pkgdir = "bicmond"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/bicmond
  install -d ${D}${sysconfdir}/bicmond
  install -m 755 setup-bic-monitor.sh ${D}${sysconfdir}/init.d/setup-bic-monitor.sh
  install -m 755 run-bic-monitor.sh ${D}${sysconfdir}/sv/bicmond/run
  update-rc.d -r ${D} setup-bic-monitor.sh start 91 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -m 644 bicmond.service ${D}${systemd_system_unitdir}
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

FILES:${PN} = "${FBPACKAGEDIR}/bicmond ${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} = "bicmond.service"
