# Copyright 2019-present Facebook. All Rights Reserved.
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

SUMMARY = "PEM Utility"
DESCRIPTION = "Utility to update PEM and get PEM EEPROM."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pem-util.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"


SRC_URI = "file://utils \
          "

S = "${WORKDIR}/utils"

LDFLAGS = "-lfruid -lpal -lpem "

DEPENDS += "libpem"
RDEPENDS:${PN} += "libpem"

do_install() {
  install -d ${D}${bindir}
  install -m 755 pem-util ${D}${bindir}/pem-util
}

FILES:${PN} = "${bindir}"
