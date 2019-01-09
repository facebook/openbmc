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
DESCRIPTION = "Daemon for monitoring the gpio sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://gpiod.c;beginline=4;endline=16;md5=5d75ad6348d98a3f7ee7e2be8db29e00"

SRC_URI = "file://Makefile \
           file://gpiod.c \
           file://setup-gpiod.sh \
           file://run-gpiod.sh \
          "

S = "${WORKDIR}"

binfiles = "gpiod \
           "

LDFLAGS += " -lbic -lminipack_gpio -lpal "

DEPENDS += " libbic libminipack-gpio libpal "
RDEPENDS_${PN} += " libbic libminipack-gpio libpal "

pkgdir = "gpiod"

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
  install -d ${D}${sysconfdir}/sv/gpiod
  install -d ${D}${sysconfdir}/gpiod
  install -m 755 setup-gpiod.sh ${D}${sysconfdir}/init.d/setup-gpiod.sh
  install -m 755 run-gpiod.sh ${D}${sysconfdir}/sv/gpiod/run
  update-rc.d -r ${D} setup-gpiod.sh start 91 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/gpiod ${prefix}/local/bin ${sysconfdir} "


INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
