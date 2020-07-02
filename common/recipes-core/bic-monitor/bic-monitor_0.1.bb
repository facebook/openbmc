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

SUMMARY = "GPIO Sensor Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the bic sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-monitor.c;beginline=4;endline=16;md5=5d75ad6348d98a3f7ee7e2be8db29e00"

SRC_URI = "file://Makefile \
           file://bic-monitor.c \
           file://setup-bic-monitor.sh \
           file://run-bic-monitor.sh \
          "

S = "${WORKDIR}"

binfiles = "bicmond \
           "

CFLAGS += " -Werror "
LDFLAGS += " -lbic -lpal -llog "

DEPENDS += " libbic libpal liblog update-rc.d-native"
RDEPENDS_${PN} += " libbic libpal liblog "

pkgdir = "bicmond"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/bicmond
  install -d ${D}${sysconfdir}/bicmond
  install -m 755 setup-bic-monitor.sh ${D}${sysconfdir}/init.d/setup-bic-monitor.sh
  install -m 755 run-bic-monitor.sh ${D}${sysconfdir}/sv/bicmond/run
  update-rc.d -r ${D} setup-bic-monitor.sh start 91 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/bicmond ${prefix}/local/bin ${sysconfdir} "


INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
