# Copyright 2016-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

LIC_FILES_CHKSUM = "file://fand.cpp;beginline=4;endline=16;md5=77b71fafe1f586d89f0450eb5585d656"

SRC_URI += "file://setup-fan.sh \
            file://fand.cpp \
           "

LDFLAGS += " -lwatchdog -lmisc-utils -lobmc-i2c -lobmc-pmbus"
RDEPENDS_${PN} += " libwatchdog libmisc-utils libobmc-i2c libobmc-pmbus"
DEPENDS_append = " update-rc.d-native libwatchdog libmisc-utils libobmc-i2c libobmc-pmbus"

CXXFLAGS_prepend = "-DCONFIG_GALAXY100 "


do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-fan.sh ${D}${sysconfdir}/init.d/setup-fan.sh
  update-rc.d -r ${D} setup-fan.sh start 91 S .
}

FILES_${PN} += "${sysconfdir}"
