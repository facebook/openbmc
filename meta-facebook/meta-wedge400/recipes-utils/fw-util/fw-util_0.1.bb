#
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
#

SUMMARY = "Firmware Utility"
DESCRIPTION = "Util for printing or updating firmware images"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fw-util.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"

SRC_URI = "file://fw-util \
          "

S = "${WORKDIR}/fw-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 fw-util ${D}${bindir}/fw-util
}

DEPENDS += "libbic libpal libipmb libipmi"
RDEPENDS_${PN} += "libbic libpal libipmb libipmi"

FILES_${PN} = "${bindir}"
