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
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmid.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

LDFLAGS_append = " -lwedge_eeprom"

DEPENDS_append = "libwedge-eeprom update-rc.d-native"
RDEPENDS_${PN} += "libwedge-eeprom"


SRC_URI = "file://Makefile \
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
  install -m 755 setup-ipmid.sh ${D}${sysconfdir}/init.d/setup-ipmid.sh
  update-rc.d -r ${D} setup-ipmid.sh start 64 S .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ipmid ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
