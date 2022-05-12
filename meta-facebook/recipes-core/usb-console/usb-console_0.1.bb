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
SUMMARY = "Set up a USB serial console"
DESCRIPTION = "Sets up a USB serial console"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://usbcons.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

DEPENDS:append = " update-rc.d-native"

LOCAL_URI = " \
    file://usbcons.sh \
    file://usbmon.sh \
    file://usbcons.service \
    "

inherit systemd

sysv_install() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 usbcons.sh ${D}${sysconfdir}/init.d/usbcons.sh
    update-rc.d -r ${D} usbcons.sh start 90 S .
}

systemd_install() {
    install -d "${D}${systemd_system_unitdir}"
    install -m 0644 usbcons.service ${D}${systemd_system_unitdir}/usbcons.service
}

do_install() {
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  install -m 755 usbmon.sh ${localbindir}/usbmon.sh

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',  d)}; then
      systemd_install
  else
      sysv_install
  fi
}

FILES:${PN} = " ${sysconfdir} /usr/local"
FILES:${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_system_unitdir}', '', d)}"
SYSTEMD_SERVICE:${PN} = "usbcons.service"
