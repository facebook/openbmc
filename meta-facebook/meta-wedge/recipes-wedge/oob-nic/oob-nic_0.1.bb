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

SUMMARY = "OOB Shared NIC driver"
DESCRIPTION = "The shared-nic driver"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://main.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://src \
          "

S = "${WORKDIR}/src"

DEPENDS += "openbmc-utils liblog libwedge-eeprom obmc-i2c"
DEPENDS += "update-rc.d-native"

RDEPENDS_${PN} += "libwedge-eeprom"

do_install() {
  install -d ${D}${sbindir}
  install -m 755 oob-nic ${D}${sbindir}/oob-nic
  install -m 755 i2craw ${D}${sbindir}/i2craw
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 etc/oob-nic.sh ${D}${sysconfdir}/init.d/oob-nic.sh
  update-rc.d -r ${D} oob-nic.sh start 80 S .
}

FILES_${PN} = " ${sbindir} ${sysconfdir} "
