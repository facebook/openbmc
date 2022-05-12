# Copyright 2014-present Facebook. All Rights Reserved.
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
SUMMARY = "Configure the sensors"
DESCRIPTION = "The script configure sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sensor-setup.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

inherit systemd

DEPENDS:append = " update-rc.d-native"

LOCAL_URI = " \
    file://sensor-setup.sh \
    file://sensor-setup.service \
    "

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 sensor-setup.sh ${D}${sysconfdir}/init.d/sensor-setup.sh
  update-rc.d -r ${D} sensor-setup.sh start 90 S .
}

install_systemd() {
  localbindir="/usr/local/bin"
  install -d ${D}${localbindir}
  install -d ${D}${systemd_system_unitdir}
  install -m 755 sensor-setup.sh ${D}${localbindir}
  install -m 644 ${S}/sensor-setup.service ${D}${systemd_system_unitdir}
}

do_install() {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
}

FILES:${PN} = " ${sysconfdir} /usr/local "

SYSTEMD_SERVICE:${PN} = "sensor-setup.service"
