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
#

SUMMARY = "Watchdog daemon"
DESCRIPTION = "Util for petting watchdog"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://watchdogd.sh;beginline=4;endline=16;md5=c5df8524e560f89f6fe75bb131d6e14d"

SRC_URI = "file://watchdogd.sh \
           file://setup-watchdogd.sh \
          "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${bindir}
  install -m 0755 watchdogd.sh ${D}${bindir}/watchdogd.sh

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-watchdogd.sh ${D}${sysconfdir}/init.d/setup-watchdogd.sh
  update-rc.d -r ${D} setup-watchdogd.sh start 95 2 3 4 5  .
}
FILES_${PN} = "${bindir}"
FILES_${PN} += "${sysconfdir}"
