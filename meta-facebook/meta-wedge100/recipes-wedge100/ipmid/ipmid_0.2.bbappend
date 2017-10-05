# Copyright 2017-present Facebook. All Rights Reserved.
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

SUMMARY = "ipmi daemon for Wedge-100"
DESCRIPTION = "ipmi daemon for Wedge-100, uses LPC bus"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmid.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

LDFLAGS_append = " -lwedge_eeprom"

DEPENDS_append = "libwedge-eeprom libipmi obmc-i2c obmc-pal update-rc.d-native"
RDEPENDS_${PN} += "libipmi libkv libwedge-eeprom"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://Makefile \
           file://ipmid.c \
           file://setup-ipmid.sh \
           file://run-ipmid.sh \
           file://sensor.c \
           file://fruid.c \
           file://lan.c \
          "
S = "${WORKDIR}"

binfiles = "ipmid"

pkgdir = "ipmid"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ipmid ${dst}/ipmid
  ln -snf ../fbpackages/${pkgdir}/ipmid ${bin}/ipmid
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmid
  install -d ${D}${sysconfdir}/ipmid
  install -m 755 setup-ipmid.sh ${D}${sysconfdir}/init.d/setup-ipmid.sh
  install -m 755 run-ipmid.sh ${D}${sysconfdir}/sv/ipmid/run
  update-rc.d -r ${D} setup-ipmid.sh start 64 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES_${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "
