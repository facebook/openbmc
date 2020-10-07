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
SUMMARY = "Detect ltc4151 behind i2c mux"
DESCRIPTION = "Detect ltc4151 behind i2c mux"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://psumuxmon.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://psumuxmon.py \
           file://psumuxmon_service \
          "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/psumuxmon
  install -m 755 psumuxmon.py ${D}${sysconfdir}/sv/psumuxmon/run
  install -m 755 psumuxmon_service ${D}${sysconfdir}/init.d/psumuxmon
  update-rc.d -r ${D} psumuxmon start 95 2 3 4 5  .
}

FILES_${PN} = "${sysconfdir} "
