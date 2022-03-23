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
LIC_FILES_CHKSUM = "file://gpiointrd.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://gpiointrd.c \
    file://setup-gpiointrd.sh \
    file://run-gpiointrd.sh \
    file://server-init.sh \
    "

DEPENDS += " libpal update-rc.d-native libgpio-ctrl libfby3-common libipmi libfby3-gpio libobmc-i2c libkv libbic libpal libras"
RDEPENDS:${PN} += " libpal libgpio-ctrl libfby3-common libipmi libfby3-gpio libobmc-i2c libkv libbic libpal libras"
CFLAGS += " -DCONFIG_FBY3 "
LDFLAGS += " -lpal -lgpio-ctrl -lfby3_common -lipmi -lbic -lfby3_gpio -lobmc-i2c -lkv -lpal -lras"

pkgdir = "gpiointrd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 gpiointrd ${dst}/gpiointrd
  ln -snf ../fbpackages/${pkgdir}/gpiointrd ${bin}/gpiointrd

  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/gpiointrd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 server-init.sh ${D}${sysconfdir}/init.d/server-init.sh
  install -m 755 setup-gpiointrd.sh ${D}${sysconfdir}/init.d/setup-gpiointrd.sh
  install -m 755 run-gpiointrd.sh ${D}${sysconfdir}/sv/gpiointrd/run
  update-rc.d -r ${D} setup-gpiointrd.sh start 91 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/gpiointrd ${prefix}/local/bin ${sysconfdir} "
