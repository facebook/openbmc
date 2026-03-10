# Copyright 2021-present Facebook. All Rights Reserved.
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
    file://setup-pemd.sh \
    file://run-pemd.sh \
    file://platform_pemd.h \
    file://platform_pemd.c \
    file://pemd.service \
    file://check-pem-present.sh \
    "

binfiles = "pemd \
           "

CFLAGS += " -llog -lpal -lpem "

DEPENDS += " liblog libpal libpem update-rc.d-native "
RDEPENDS:${PN} += " liblog libpal libpem "

pkgdir = "pemd"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/pemd
  install -d ${D}${sysconfdir}/pemd
  install -m 755 setup-pemd.sh ${D}${sysconfdir}/init.d/setup-pemd.sh
  install -m 755 run-pemd.sh ${D}${sysconfdir}/sv/pemd/run
  update-rc.d -r ${D} setup-pemd.sh start 95 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -m 0644 ${UNPACKDIR}/pemd.service ${D}${systemd_system_unitdir}
  install -d ${D}${prefix}/local/bin
  install -m 0755 ${UNPACKDIR}/check-pem-present.sh ${D}${prefix}/local/bin/check-pem-present.sh
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

FILES:${PN} = "${FBPACKAGEDIR}/pemd ${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} += "pemd.service"
