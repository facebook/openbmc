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

SUMMARY = "IPMI Daemon"
DESCRIPTION = "Daemon to handle IPMI Messages."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ipmid.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

LDFLAGS:append = " -lwedge_eeprom -llog"

DEPENDS:append = "libwedge-eeprom liblog update-rc.d-native"
RDEPENDS:${PN} += "libwedge-eeprom liblog"

inherit systemd

LOCAL_URI = " \
    file://Makefile \
    file://setup-ipmid.sh \
    file://ipmid.c \
    file://platform/timestamp.c \
    file://platform/timestamp.h \
    file://platform/sel.c \
    file://platform/sel.h \
    file://platform/sdr.c \
    file://platform/sdr.h \
    file://platform/sensor.h \
    file://platform/fruid.h \
    file://platform/wedge/sensor.c \
    file://platform/wedge/fruid.c \
    file://ipmid.service \
    "

binfiles = "ipmid"

pkgdir = "ipmid"

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 setup-ipmid.sh ${D}${sysconfdir}/init.d/setup-ipmid.sh
    update-rc.d -r ${D} setup-ipmid.sh start 64 S .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 ipmid.service ${D}${systemd_system_unitdir}
}

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ipmid ${dst}/ipmid
  ln -snf ../fbpackages/${pkgdir}/ipmid ${bin}/ipmid

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} = "ipmid.service"
