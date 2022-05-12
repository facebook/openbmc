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

SUMMARY = "Bridge IC Cache Daemon"
DESCRIPTION = "Daemon to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-cache.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile \
    file://setup-bic-cache.sh \
    file://bic-cache.c \
    file://setup_bic_cache.service \
    "

LDFLAGS = "-lbic -lpal -llog"

binfiles = "bic-cache"

pkgdir = "bic-cache"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-bic-cache.sh ${D}${sysconfdir}/init.d/setup-bic-cache.sh
  update-rc.d -r ${D} setup-bic-cache.sh start 66 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -m 755 setup-bic-cache.sh ${D}/usr/local/bin/setup-bic-cache.sh
  install -m 644 setup_bic_cache.service ${D}${systemd_system_unitdir}
}

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 bic-cache ${dst}/bic-cache
  ln -snf ../fbpackages/${pkgdir}/bic-cache ${bin}/bic-cache

  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install_systemd
  else
      install_sysv
  fi

}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/bic-cache ${prefix}/local/bin ${sysconfdir} "

DEPENDS += " libbic libpal liblog update-rc.d-native"
RDEPENDS:${PN} += " libbic libpal liblog bash"

SYSTEMD_SERVICE:${PN} = "setup_bic_cache.service"
