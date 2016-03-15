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
SUMMARY = "Tool to BMC's password"
DESCRIPTION = "Stores and retrieve the password from persistent storage"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://passwd-util;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"


DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://passwd-util \
          "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 passwd-util ${D}${bindir}/passwd-util
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/default
  install -m 755 passwd-util ${D}${sysconfdir}/init.d/passwd-util
  update-rc.d -r ${D} passwd-util start 05 S 5 .
}

FILES_${PN} = "${prefix}/local/bin ${sysconfdir} ${bindir}"
