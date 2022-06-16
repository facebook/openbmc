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

SUMMARY = "GPIO Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the gpio signals"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://gpiod.c;beginline=4;endline=16;md5=3598c23c3531e1f059568c83d4174cc0"

LOCAL_URI = " \
    file://Makefile \
    file://gpiod.c \
    file://setup-gpiod.sh \
    "

binfiles = "gpiod \
           "
DEPENDS += "libpal libgpio-ctrl update-rc.d-native libnm libkv libobmc-i2c libpeci-sensors"
RDEPENDS:${PN} += "libpal libnm libgpio-ctrl libkv libobmc-i2c libpeci-sensors"
LDFLAGS += "-pthread -lpal -lnm -lgpio-ctrl -lkv -lobmc-i2c -lpeci_sensors"

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
  install -m 755 setup-gpiod.sh ${D}${sysconfdir}/init.d/setup-gpiod.sh
  update-rc.d -r ${D} setup-gpiod.sh start 92 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/gpiod ${prefix}/local/bin ${sysconfdir} "

