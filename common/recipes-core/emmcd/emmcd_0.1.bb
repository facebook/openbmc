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

inherit systemd

SUMMARY = "eMMC Daemon"
DESCRIPTION = "Daemon to monitor eMMC status "
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://emmcd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

RDEPENDS:${PN} += "liblog libmisc-utils"
DEPENDS:append = " update-rc.d-native liblog libmisc-utils"

LOCAL_URI = " \
    file://emmcd.c \
    file://meson.build \
    file://run-emmcd.sh \
    file://setup-emmcd.sh \
    file://emmcd.service \
    "

pkgdir = "emmcd"
inherit legacy-packages

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/emmcd
  install -m 755 ${S}/setup-emmcd.sh ${D}${sysconfdir}/init.d/setup-emmcd.sh
  install -m 755 ${S}/run-emmcd.sh ${D}${sysconfdir}/sv/emmcd/run
  update-rc.d -r ${D} setup-emmcd.sh start 99 5 .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 755 ${S}/setup-emmcd.sh ${D}/usr/local/bin/setup-emmcd.sh

    sed -i -e '/runsv/d' ${D}/usr/local/bin/setup-emmcd.sh

    install -m 644 ${S}/emmcd.service ${D}${systemd_system_unitdir}
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi
}

SYSTEMD_SERVICE:${PN} = "emmcd.service"
