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
SUMMARY = "Tool to restore configure"
DESCRIPTION = "restore configure files"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://setup-conf.sh;beginline=3;endline=18;md5=700bc730f27f8d9b05ac017220c137e5"

DEPENDS:append = " update-rc.d-native"

LOCAL_URI = " \
    file://setup-conf.sh \
    "

do_install() {
  install -d ${D}${bindir}
  install -m 0755 setup-conf.sh ${D}${bindir}/setup-conf.sh
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-conf.sh ${D}${sysconfdir}/init.d/setup-conf.sh
  update-rc.d -r ${D} setup-conf.sh start 03 S 5 .
}

FILES:${PN} = "${prefix}/local/bin ${sysconfdir} ${bindir}"
