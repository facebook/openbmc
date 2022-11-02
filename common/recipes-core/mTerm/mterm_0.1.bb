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
#

SUMMARY = "Terminal Multiplexer"
DESCRIPTION = "Util for multiplexing terminal"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mTerm_server.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

inherit systemd
LOCAL_URI = " \
    file://mTerm_server.c \
    file://mTerm_client.c \
    file://mTerm_helper.c \
    file://mTerm_helper.h \
    file://tty_helper.c \
    file://tty_helper.h \
    file://meson.build \
    file://mTerm/run \
    file://mTerm-service-setup.sh \
    "

inherit meson pkgconfig
inherit legacy-packages

CONS_BIN_FILES = "mTerm_server \
                  mTerm_client \
                 "
# If the target system has more than one service,
# It may override this variable in it's bbappend
# file. Also the platform must provide those in
# its appended package.
MTERM_SERVICES ?= "mTerm"
MTERM_SYSTEMD_SERVICES ?= "mTerm_server.service"
SYSTEMD_SERVICE:${PN} = "${MTERM_SYSTEMD_SERVICES}"

pkgdir = "mTerm"

DEPENDS += "update-rc.d-native"

systemd_install() {
    install -d ${D}${systemd_system_unitdir}
    for svc in ${MTERM_SYSTEMD_SERVICES}; do
        install -m 644 ${S}/$svc ${D}${systemd_system_unitdir}
    done
}

sysv_install() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/sv
    for svc in ${MTERM_SERVICES}; do
        install -d ${D}${sysconfdir}/sv/${svc}
        install -m 755 ${S}/${svc}/run ${D}${sysconfdir}/sv/${svc}/run
    done
    install -m 755 ${S}/mTerm-service-setup.sh ${D}${sysconfdir}/init.d/mTerm-service-setup.sh
    update-rc.d -r ${D} mTerm-service-setup.sh start 84 S .
}

do_install:append() {
  for svc in ${MTERM_SERVICES}; do
      install -d ${D}${sysconfdir}/${svc}
  done
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',  d)}; then
      systemd_install
  else
      sysv_install
  fi
}
